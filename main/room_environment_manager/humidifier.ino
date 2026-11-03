#include <esp_now.h>
#include <WiFi.h>
#include <stdlib.h>
#include "humidifier.h"
#include "additional_sensors.h"
#include "memory.h"

namespace {
#ifdef HUMIDIFIER_MAC_ADDR
const uint8_t hm_mac[6] = HUMIDIFIER_MAC_ADDR;
const bool hm_enabled = true;
#else
const uint8_t hm_mac[6] = {};
const bool hm_enabled = false;
#endif
constexpr uint32_t HM_STATUS_MAX_AGE_MS = 180000;
portMUX_TYPE hm_rx_mux = portMUX_INITIALIZER_UNLOCKED;
bool hm_receiving = false;
bool hm_received = false;
HmPacket hm_rx = {};
uint32_t hm_sequence = 0;
bool hm_online = false;
uint32_t hm_last_status_ms = 0;
HmPacket hm_status = hm_packet(HM_STATUS, 0);
bool hm_setting = false;
uint8_t hm_requested_state = HM_OFF;
bool hm_command_ok = false;
bool hm_alert_sent = false;
uint32_t hm_last_alert_attempt_ms = 0;
bool hm_alert_attempted = false;
HmPid hm_pid;
bool hm_attempted_switch = false;
uint32_t hm_last_switch_ms = 0;

const char *hm_state_label(uint8_t state) {
  switch (state) {
    case HM_OFF: return "OFF";
    case HM_WEAK: return "WEAK";
    case HM_STRONG: return "STRONG";
    default: return "UNKNOWN";
  }
}

bool hm_set_state(uint8_t state) {
  if (!hm_enabled) return false;
  hm_setting = true;
  hm_requested_state = state;
  hm_command_ok = false;
  hm_attempted_switch = true;
  hm_last_switch_ms = millis();
  additional_sensors_request(true);
  hm_setting = false; // never queue a manual command for a future reconnection
  return hm_command_ok;
}

void hm_reset_control() {
  hm_pid.reset();
  hm_attempted_switch = false;
}

bool hm_parse_target(const String &text, double &value) {
  if (text.length() == 0) return false;
  char *end = nullptr;
  value = strtod(text.c_str(), &end);
  return end != text.c_str() && *end == '\0' && hm_valid_humidity(value);
}
} // namespace

bool humidifier_configured() { return hm_enabled; }

bool humidifier_connected() {
  return hm_enabled && hm_online && (uint32_t)(millis() - hm_last_status_ms) <= HM_STATUS_MAX_AGE_MS;
}

void humidifier_exchange_begin() {
  if (!hm_enabled) return;
  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, hm_mac, 6);
  peer.ifidx = WIFI_IF_STA;
  peer.channel = 0; // use the additional-sensor session's channel
  esp_err_t result = esp_now_add_peer(&peer);
  if (result != ESP_OK && result != ESP_ERR_ESPNOW_EXIST) {
    hm_online = false;
    return;
  }
  portENTER_CRITICAL(&hm_rx_mux);
  hm_sequence = esp_random();
  hm_received = false;
  hm_receiving = true;
  portEXIT_CRITICAL(&hm_rx_mux);
  HmPacket request = hm_packet(hm_setting ? HM_SET : HM_QUERY, hm_sequence);
  request.state = hm_setting ? hm_requested_state : (uint8_t)HM_UNKNOWN;
  esp_now_send(hm_mac, reinterpret_cast<const uint8_t *>(&request), sizeof(request));
}

void humidifier_receive(const uint8_t *mac, const uint8_t *data, int length) {
  if (!hm_enabled || length != sizeof(HmPacket) || memcmp(mac, hm_mac, 6) != 0) return;
  HmPacket packet;
  memcpy(&packet, data, sizeof(packet));
  if (!hm_packet_valid(packet) || packet.type != HM_STATUS) return;
  portENTER_CRITICAL(&hm_rx_mux);
  if (hm_receiving && packet.sequence == hm_sequence) {
    hm_rx = packet;
    hm_received = true;
  }
  portEXIT_CRITICAL(&hm_rx_mux);
}

void humidifier_exchange_end() {
  if (!hm_enabled) return;
  portENTER_CRITICAL(&hm_rx_mux);
  const bool received = hm_receiving && hm_received;
  const HmPacket packet = hm_rx;
  hm_receiving = false;
  portEXIT_CRITICAL(&hm_rx_mux);
  hm_online = received;
  if (!received) return; // absent is normal; no Slack warning or retry loop
  hm_status = packet;
  hm_last_status_ms = millis();
  hm_command_ok = hm_setting && packet.result == HM_OK && packet.state == hm_requested_state;
}

void humidifier_update(Settings &settings, const Sensor_data &sensors, Time_info &time) {
  if (!settings.hm_water_alert) hm_alert_sent = false;
  if (!humidifier_connected()) {
    hm_pid.reset(); // no integral accumulation while the optional device is absent
    return;
  }
  if (!hm_status.needs_water) {
    hm_alert_sent = false;
    hm_alert_attempted = false;
  } else if (settings.hm_water_alert && !hm_alert_sent &&
             (!hm_alert_attempted || (uint32_t)(millis() - hm_last_alert_attempt_ms) >= 60000)) {
    String message = "[ALERT] Humidifier needs water.";
    if (settings.hm_water_mention) message = String("<!channel>\n") + message;
    hm_alert_attempted = true;
    hm_last_alert_attempt_ms = millis();
    const char *ts = slack_send_message(time, message);
    hm_alert_sent = ts && ts[0] != '\0';
  }
  if (!settings.hm_auto) {
    hm_pid.reset();
    return;
  }
  const double humidity = sensors.representative.humidity;
  const bool can_switch = !hm_attempted_switch || (uint32_t)(millis() - hm_last_switch_ms) >= HM_SWITCH_INTERVAL_MS;
  if (!hm_valid_humidity(humidity) || !hm_valid_humidity(settings.hm_target_humidity) ||
      hm_status.needs_water || hm_status.state == HM_UNKNOWN) {
    hm_pid.reset();
    // Fail closed on missing humidity; failures still have a bounded retry cadence.
    if (hm_status.state != HM_OFF && can_switch) hm_set_state(HM_OFF);
    return;
  }
  hm_pid.update(settings.hm_target_humidity, humidity, millis());
  const uint8_t desired = hm_pid.state(hm_status.state);
  if (desired != hm_status.state && can_switch) hm_set_state(desired);
  else if (!hm_attempted_switch) {
    // The device may already be running when auto starts; hold that initial state too.
    hm_attempted_switch = true;
    hm_last_switch_ms = millis();
  }
}

String humidifier_summary(const Settings &settings) {
  String text = "< *Humidifier* >\n- Status : ";
  text += humidifier_connected() ? hm_state_label(hm_status.state) : "UNKNOWN";
  text += "\n- Auto PID : ";
  text += settings.hm_auto ? String(settings.hm_target_humidity, 1) + " %" : "OFF";
  text += "\n- Needs water : ";
  text += humidifier_connected() ? (hm_status.needs_water ? "YES" : "NO") : "UNKNOWN";
  text += String("\n- Refill alert : ") + (settings.hm_water_alert ? "ON" : "OFF");
  text += String(" / mention : ") + (settings.hm_water_mention ? "ON\n" : "OFF\n");
  return text;
}

void command_check_hm(Command command, Time_info &time, Settings &settings, const Sensor_data &sensors) {
  if (command.cmd != "hm") return;
  const bool extra = command.arg3.length() != 0 || command.arg4.length() != 0;
  if (!extra && (command.arg1 == "alert" || command.arg1 == "mention") &&
      (command.arg2 == "on" || command.arg2 == "off")) {
    const bool enabled = command.arg2 == "on";
    if (command.arg1 == "alert") {
      settings.hm_water_alert = enabled;
      hm_alert_sent = false;
      hm_alert_attempted = false;
    } else settings.hm_water_mention = enabled;
    bool saved = memory_save_settings(settings);
    slack_send_message(time, String(saved ? "[INFO] HM " : "[WARN] Settings active but could not save: HM ") +
                       command.arg1 + " " + command.arg2);
    return;
  }
  if (!extra && (command.arg1 == "a" || command.arg1 == "auto")) {
    double target;
    if (!hm_parse_target(command.arg2, target)) {
      slack_send_message(time, "[ERROR] Expected hm a [humidity 0-100].");
      return;
    }
    settings.hm_auto = true;
    settings.hm_target_humidity = target;
    hm_reset_control();
    bool saved = memory_save_settings(settings);
    humidifier_update(settings, sensors, time);
    slack_send_message(time, String(saved ? "[INFO] " : "[WARN] Could not save settings. ") +
                       "HM AUTO PID target: " + String(target, 1) + " %" +
                       (!hm_enabled ? " (controller not configured)" :
                        (!humidifier_connected() ? " (waiting for controller)" : "")));
    return;
  }
  uint8_t desired = HM_UNKNOWN;
  if (!extra && command.arg1 == "off" && command.arg2.length() == 0) desired = HM_OFF;
  if (!extra && command.arg1 == "on") {
    if (command.arg2 == "w" || command.arg2 == "weak") desired = HM_WEAK;
    if (command.arg2 == "s" || command.arg2 == "strong") desired = HM_STRONG;
  }
  if (desired == HM_UNKNOWN) {
    slack_send_message(time, "[ERROR] Expected hm on [w/weak/s/strong], hm off, hm a [0-100], hm alert [on/off], or hm mention [on/off].");
    return;
  }
  settings.hm_auto = false;
  hm_reset_control();
  bool saved = memory_save_settings(settings);
  const bool success = hm_set_state(desired);
  String message = success ? String("[INFO] HM ") + hm_state_label(desired) :
    (!hm_enabled ? String("[WARN] HM controller not configured (HUMIDIFIER_MAC_ADDR).") :
     String("[WARN] HM command not confirmed. Check connection, water and LEDs; retry if needed."));
  message += " Auto disabled.";
  if (!saved) message += " Could not save settings.";
  slack_send_message(time, message);
}
