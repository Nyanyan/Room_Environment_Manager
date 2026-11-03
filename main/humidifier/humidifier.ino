#include <esp_now.h>
#include <esp_wifi.h>
#include <WiFi.h>
#include "humidifier_protocol.h"
#include "token.h"

// LED inputs are active LOW. Button cycles OFF -> WEAK -> STRONG -> OFF.
#define LED_NEED_WATER 17
#define LED_WORKING_WEAK 18
#define LED_WORKING_STRONG 23
#define BUTTON 4
#define LED_STATE_LIGHT LOW
#ifndef HUMIDIFIER_BUTTON_ACTIVE_LEVEL
#define HUMIDIFIER_BUTTON_ACTIVE_LEVEL HIGH
#endif

constexpr uint8_t HM_CHANNEL = 1;
constexpr uint32_t HM_BUTTON_MS = 200;
constexpr uint32_t HM_SETTLE_MS = 250;
constexpr uint32_t HM_DEBOUNCE_MS = 80;
constexpr uint32_t HM_LEASE_MS = 300000;
const uint8_t parent_mac[6] = HUMIDIFIER_PARENT_MAC_ADDR;

portMUX_TYPE request_mux = portMUX_INITIALIZER_UNLOCKED;
HmPacket incoming_request = {};
bool request_pending = false;
bool radio_ready = false;
uint32_t last_radio_attempt_ms = 0;
uint8_t raw_leds = 0xFF;
uint8_t stable_state = HM_UNKNOWN;
bool stable_water = false;
bool leds_ready = false;
uint32_t raw_leds_since = 0;
bool controlling = false;
bool button_pressed = false;
bool settling = false;
uint32_t button_since = 0;
uint32_t settle_since = 0;
uint8_t requested_state = HM_OFF;
uint8_t presses = 0;
uint32_t command_sequence = 0;
bool have_command = false;
uint8_t command_result = HM_IDLE;
bool remote_lease = false;
uint32_t last_parent_ms = 0;

void hm_release_button() {
  digitalWrite(BUTTON, HUMIDIFIER_BUTTON_ACTIVE_LEVEL == HIGH ? LOW : HIGH);
  button_pressed = false;
}

void hm_read_leds(uint32_t now) {
  const bool weak = digitalRead(LED_WORKING_WEAK) == LED_STATE_LIGHT;
  const bool strong = digitalRead(LED_WORKING_STRONG) == LED_STATE_LIGHT;
  const bool water = digitalRead(LED_NEED_WATER) == LED_STATE_LIGHT;
  const uint8_t sample = (weak ? 1 : 0) | (strong ? 2 : 0) | (water ? 4 : 0);
  if (sample != raw_leds) {
    raw_leds = sample;
    raw_leds_since = now;
  }
  if ((uint32_t)(now - raw_leds_since) < HM_DEBOUNCE_MS) return;
  stable_state = weak && strong ? HM_UNKNOWN : (strong ? HM_STRONG : (weak ? HM_WEAK : HM_OFF));
  stable_water = water;
  leds_ready = true;
}

void hm_reply(uint32_t sequence, uint8_t result) {
  if (!radio_ready) return;
  HmPacket packet = hm_packet(HM_STATUS, sequence);
  packet.state = stable_state;
  packet.needs_water = stable_water;
  packet.result = result;
  esp_now_send(parent_mac, reinterpret_cast<const uint8_t *>(&packet), sizeof(packet));
}

void hm_finish(uint8_t result) {
  hm_release_button();
  controlling = false;
  settling = false;
  command_result = result;
  hm_reply(command_sequence, result);
}

void hm_start(uint8_t target, uint32_t sequence) {
  hm_release_button();
  requested_state = target;
  command_sequence = sequence;
  have_command = true;
  presses = 0;
  settling = false;
  controlling = true;
  command_result = HM_BUSY;
}

void hm_drive_button(uint32_t now) {
  if (!controlling) return;
  if (button_pressed) {
    if ((uint32_t)(now - button_since) >= HM_BUTTON_MS) {
      hm_release_button();
      settling = true;
      settle_since = now;
    }
    return;
  }
  if (settling) {
    if ((uint32_t)(now - settle_since) < HM_SETTLE_MS) return;
    settling = false;
  }
  if (!leds_ready || stable_state == HM_UNKNOWN) {
    hm_finish(HM_FAILED);
  } else if (stable_water && requested_state != HM_OFF) {
    hm_finish(HM_EMPTY);
  } else if (stable_state == requested_state) {
    hm_finish(HM_OK); // success comes from LED feedback, not radio delivery
  } else if (presses >= 3) {
    hm_finish(HM_FAILED); // stuck/miswired LEDs never cause endless button presses
  } else {
    digitalWrite(BUTTON, HUMIDIFIER_BUTTON_ACTIVE_LEVEL);
    button_pressed = true;
    button_since = now;
    ++presses;
  }
}

void OnHumidifierDataRecv(const uint8_t *mac, const uint8_t *data, int length) {
  if (length != sizeof(HmPacket) || memcmp(mac, parent_mac, 6) != 0) return;
  HmPacket packet;
  memcpy(&packet, data, sizeof(packet));
  if (!hm_packet_valid(packet) || (packet.type != HM_QUERY && packet.type != HM_SET) ||
      (packet.type == HM_SET && packet.state > HM_STRONG)) return;
  portENTER_CRITICAL(&request_mux);
  incoming_request = packet;
  request_pending = true;
  portEXIT_CRITICAL(&request_mux);
}

void hm_process_request(uint32_t now) {
  portENTER_CRITICAL(&request_mux);
  const bool pending = request_pending;
  const HmPacket request = incoming_request;
  request_pending = false;
  portEXIT_CRITICAL(&request_mux);
  if (!pending) return;
  last_parent_ms = now;
  if (request.type == HM_QUERY) {
    hm_reply(request.sequence, controlling ? HM_BUSY : command_result);
  } else if (have_command && request.sequence == command_sequence) {
    hm_reply(request.sequence, command_result); // retry is idempotent
  } else {
    remote_lease = request.state != HM_OFF;
    hm_start(request.state, request.sequence);
    hm_reply(request.sequence, HM_BUSY);
  }
}

void hm_init_radio() {
  last_radio_attempt_ms = millis();
  if (esp_now_init() != ESP_OK) return;
  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, parent_mac, 6);
  peer.channel = HM_CHANNEL;
  peer.ifidx = WIFI_IF_STA;
  if (esp_now_add_peer(&peer) != ESP_OK || esp_now_register_recv_cb(OnHumidifierDataRecv) != ESP_OK) {
    esp_now_deinit();
    return;
  }
  radio_ready = true;
}

void setup() {
  // Set the idle level before enabling the output to avoid an unintended press.
  hm_release_button();
  pinMode(BUTTON, OUTPUT);
  pinMode(LED_NEED_WATER, INPUT);
  pinMode(LED_WORKING_WEAK, INPUT);
  pinMode(LED_WORKING_STRONG, INPUT);
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  esp_wifi_set_channel(HM_CHANNEL, WIFI_SECOND_CHAN_NONE);
  Serial.print("Humidifier STA MAC: ");
  Serial.println(WiFi.macAddress());
  hm_init_radio();
}

void loop() {
  const uint32_t now = millis();
  hm_read_leds(now);
  hm_process_request(now);
  // Stop remote operation after a prolonged parent outage. No reboot/retry loop.
  if (remote_lease && (uint32_t)(now - last_parent_ms) >= HM_LEASE_MS) {
    remote_lease = false;
    hm_start(HM_OFF, command_sequence);
  }
  hm_drive_button(now);
  if (!radio_ready && (uint32_t)(now - last_radio_attempt_ms) >= 5000) hm_init_radio();
  delay(1);
}
