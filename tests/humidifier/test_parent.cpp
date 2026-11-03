#include <cassert>
#include <iostream>
#include "humidifier.h"

std::vector<std::string> messages;
char *slack_send_message(Time_info &, String message) {
  messages.emplace_back(message.c_str());
  static char ts[] = "123.456";
  return ts;
}
#include "humidifier.ino"
#include "memory.ino"

bool device_online = true;
bool device_water = false;
bool stale_reply = false;
bool wrong_mac = false;
uint8_t device_state = HM_OFF;
int requests = 0;
void additional_sensors_request(bool) {
  ++requests;
  humidifier_exchange_begin();
  if (device_online && !fake_packets.empty()) {
    HmPacket request;
    memcpy(&request, fake_packets.back().data(), sizeof(request));
    HmPacket reply = hm_packet(HM_STATUS, request.sequence + (stale_reply ? 1 : 0));
    if (request.type == HM_SET && (!device_water || request.state == HM_OFF)) device_state = request.state;
    reply.state = device_state;
    reply.needs_water = device_water;
    reply.result = device_water && request.state != HM_OFF ? HM_EMPTY : HM_OK;
    uint8_t source[6];
    memcpy(source, hm_mac, 6);
    if (wrong_mac) source[0] ^= 1;
    humidifier_receive(source, reinterpret_cast<const uint8_t *>(&reply), sizeof(reply));
  }
  humidifier_exchange_end();
}

int main() {
  Settings settings;
  Sensor_data sensors;
  sensors.representative.humidity = 40;
  Time_info time = {};
  auto command = [&](const char *action, const char *argument = "", const char *extra = "") {
    Command cmd;
    cmd.cmd = "hm"; cmd.arg1 = action; cmd.arg2 = argument; cmd.arg3 = extra;
    command_check_hm(cmd, time, settings, sensors);
  };
#ifndef HUMIDIFIER_MAC_ADDR
  command("on", "strong");
  assert(requests == 0 && !humidifier_connected());
  command("a", "50");
  assert(settings.hm_auto && requests == 0);
  std::cout << "Parent: unconfigured optional device sends no packets\n";
  return 0;
#endif
  command("on", "w"); assert(device_state == HM_WEAK && hm_command_ok);
  command("on", "strong"); assert(device_state == HM_STRONG && hm_command_ok);
  command("on", "weak"); assert(device_state == HM_WEAK);
  command("on", "s"); assert(device_state == HM_STRONG);
  command("off"); assert(device_state == HM_OFF && !settings.hm_auto);
  int before = requests;
  for (const char *bad : {"", "-1", "101", "nan", "inf", "hello", "50abc"}) {
    command("a", bad);
    assert(!settings.hm_auto && requests == before);
  }
  command("on", "bad"); command("off", "extra"); command("a", "50", "extra");
  assert(requests == before && !settings.hm_auto);
  command("a", "0"); assert(settings.hm_auto && settings.hm_target_humidity == 0);
  command("a", "100"); assert(settings.hm_target_humidity == 100 && device_state == HM_STRONG);
  command("a", "50.5"); assert(settings.hm_target_humidity == 50.5);
  // Automatic changes respect dwell, but explicit manual commands bypass it.
  before = requests;
  sensors.representative.humidity = 60;
  fake_now += HM_PID_SAMPLE_MS;
  humidifier_update(settings, sensors, time);
  assert(requests == before);
  fake_now += HM_SWITCH_INTERVAL_MS;
  additional_sensors_request(false);
  humidifier_update(settings, sensors, time);
  assert(device_state == HM_OFF);
  command("on", "weak"); assert(device_state == HM_WEAK && !settings.hm_auto);

  device_water = true;
  additional_sensors_request(false);
  messages.clear();
  humidifier_update(settings, sensors, time);
  humidifier_update(settings, sensors, time);
  assert(messages.size() == 1 && messages[0].find("<!channel>") == std::string::npos);
  device_water = false; additional_sensors_request(false); humidifier_update(settings, sensors, time);
  command("mention", "on");
  messages.clear(); device_water = true; additional_sensors_request(false);
  humidifier_update(settings, sensors, time);
  assert(messages.size() == 1 && messages[0].find("<!channel>") != std::string::npos);
  command("alert", "off"); messages.clear(); humidifier_update(settings, sensors, time);
  assert(messages.empty());
  command("on", "strong"); assert(!hm_command_ok && device_state == HM_WEAK);
  device_online = false; before = requests;
  command("off"); assert(!hm_command_ok && requests == before + 1 && !settings.hm_auto);
  command("a", "50");
  messages.clear(); before = requests;
  for (int i = 0; i < 10; ++i) humidifier_update(settings, sensors, time);
  assert(requests == before && messages.empty() && !hm_pid.initialized);
  assert(std::string(humidifier_summary(settings).c_str()).find("UNKNOWN") != std::string::npos);
  device_online = true; device_water = false;
  stale_reply = true; command("on", "weak"); assert(!hm_command_ok);
  stale_reply = false; wrong_mac = true; command("on", "weak"); assert(!hm_command_ok);
  wrong_mac = false; additional_sensors_request(false); assert(humidifier_connected());
  fake_now += HM_STATUS_MAX_AGE_MS + 1; assert(!humidifier_connected());
  additional_sensors_request(false);
  settings.hm_auto = true; sensors.representative.humidity = NAN; hm_reset_control();
  humidifier_update(settings, sensors, time);
  assert(device_state == HM_OFF && !hm_pid.initialized);
  device_water = false; sensors.representative.humidity = 30; fake_now += HM_SWITCH_INTERVAL_MS;
  additional_sensors_request(false); humidifier_update(settings, sensors, time);
  assert(device_state == HM_STRONG); // reconnect resumes auto
  for (auto state : {HM_OFF, HM_WEAK, HM_STRONG}) {
    device_state = state; additional_sensors_request(false);
    const std::string report = humidifier_summary(settings).c_str();
    assert(report.find(hm_state_label(state)) != std::string::npos);
    assert(report.find("Needs water : NO") != std::string::npos);
  }

  // Old EEPROM blocks keep their offsets and reservations when HM settings are saved.
  SettingsPersisted old = {};
  old.magic = kMagicSettings; old.version = kVersionSettings;
  old.alert_when_hot = 1; old.alert_hot_temperature = 32; old.ac_auto_temp = 23;
  old.checksum = compute_checksum(old); EEPROM.put(kSettingsAddr, old);
  std::fill(EEPROM.bytes.begin() + kHumidifierSettingsAddr, EEPROM.bytes.end(), 0xFF);
  Settings loaded; assert(memory_load_settings(loaded));
  assert(!loaded.hm_auto && loaded.hm_water_alert && !loaded.hm_water_mention);
  assert(loaded.alert_hot_temperature == 32 && loaded.ac_auto_temp == 23);
  CommandReservation reservation; reservation.year = 2027; reservation.month = 1; reservation.day = 1;
  strcpy(reservation.command, "hm on weak"); uint32_t id;
  assert(reservation_add(reservation, id));
  loaded.hm_auto = true; loaded.hm_target_humidity = 55.5;
  loaded.hm_water_alert = false; loaded.hm_water_mention = true;
  assert(memory_save_settings(loaded)); Settings restored;
  assert(memory_load_settings(restored));
  assert(restored.hm_auto && restored.hm_target_humidity == 55.5 && !restored.hm_water_alert && restored.hm_water_mention);
  CommandReservation list[RESERVATION_MAX]; size_t count;
  assert(reservation_list(list, RESERVATION_MAX, count) && count == 1 && list[0].id == id);
  EEPROM.bytes[kHumidifierSettingsAddr] ^= 1;
  assert(memory_load_settings(restored) && !restored.hm_auto && restored.hm_water_alert);
  assert(reservation_list(list, RESERVATION_MAX, count) && count == 1);
  std::cout << "Parent: commands, PID dwell, alerts, missing device, MAC/sequence checks, reports and EEPROM migration passed\n";
}
