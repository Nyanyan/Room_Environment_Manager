#include <cassert>
#include <iostream>
#include "graph.h"
#include "humidifier.h"
#include "memory.h"

using std::min;
using std::max;
template<class T> T constrain(T value, T low, T high) { return min(max(value, low), high); }

Settings settings;
std::vector<std::string> messages;
std::vector<std::string> ac_requests;
bool device_online = true;
uint8_t device_state = HM_STRONG;
int hm_requests = 0;

char *slack_send_message(Time_info &, String message) {
  messages.emplace_back(message.c_str());
  static char timestamp[] = "123.456";
  return timestamp;
}
String slack_get_message() { return ""; }
String slack_upload_img(uint8_t *, uint32_t, const char *) { return "test-image"; }
char *slack_send_environment_report(Time_info &time, const Sensor_data &, const String &heading, const String &details) {
  return slack_send_message(time, heading + details);
}
bool additional_sensor_received(int) { return false; }
bool ac_controller_check_connection() { return true; }
struct FakeESP { void restart() { assert(false); } } ESP;

void ac_cool_on(AC_status &ac, int temperature) {
  ac_requests.emplace_back("cool"); ac.state = AC_STATE_COOL; ac.temp = temperature;
  memory_save_ac_status(ac);
}
void ac_dry_on(AC_status &ac, int temperature) {
  ac_requests.emplace_back("dry"); ac.state = AC_STATE_DRY; ac.temp = temperature;
  memory_save_ac_status(ac);
}
void ac_heat_on(AC_status &ac, int temperature) {
  ac_requests.emplace_back("heat"); ac.state = AC_STATE_HEAT; ac.temp = temperature;
  memory_save_ac_status(ac);
}
void ac_off(AC_status &ac) {
  ac_requests.emplace_back("off"); ac.state = AC_STATE_OFF;
  memory_save_ac_status(ac);
}

#include "memory.ino"
#include "humidifier.ino"
#include "graph.ino"
String get_graph_urls(Graph_data &, Graph_img &, Time_info &);
#include "command.ino"

void additional_sensors_request(bool) {
  ++hm_requests;
  humidifier_exchange_begin();
  if (device_online && !fake_packets.empty()) {
    HmPacket request;
    memcpy(&request, fake_packets.back().data(), sizeof(request));
    HmPacket reply = hm_packet(HM_STATUS, request.sequence);
    if (request.type == HM_SET) device_state = request.state;
    reply.state = device_state;
    reply.result = HM_OK;
    humidifier_receive(hm_mac, reinterpret_cast<const uint8_t *>(&reply), sizeof(reply));
  }
  humidifier_exchange_end();
}

static Sensor_data sensors;
static AC_status ac;
static Graph_data data;
static Graph_img image;
static Time_info now = {};
static void run(const char *text) {
  command_process(parse_command_text(text), now, sensors, settings, ac, data, image);
}

static void test_all_off() {
  settings.ac_auto_mode = AC_AUTO_HEAT; settings.hm_auto = true;
  ac.state = AC_STATE_HEAT;
  run("all off");
  assert(ac.state == AC_STATE_OFF && device_state == HM_OFF);
  assert(settings.ac_auto_mode == AC_AUTO_OFF && !settings.hm_auto);
  assert(ac_requests.size() == 1 && ac_requests.back() == "off" && hm_requests == 1);
  Settings restored;
  assert(memory_load_settings(restored) && restored.ac_auto_mode == AC_AUTO_OFF && !restored.hm_auto);

  // An absent humidifier still gets an attempted stop and cannot restart via PID.
  device_online = false;
  settings.ac_auto_mode = AC_AUTO_COOL; settings.hm_auto = true;
  run("all off");
  assert(ac_requests.size() == 2 && hm_requests == 2);
  assert(settings.ac_auto_mode == AC_AUTO_OFF && !settings.hm_auto && !hm_command_ok);
  assert(messages.back().find("not confirmed") != std::string::npos);
  device_online = true;
  for (const char *invalid : {"all", "all on", "all off extra", "all off ac hm"}) run(invalid);
  assert(ac_requests.size() == 2 && hm_requests == 2);
}

static void test_preset_storage() {
  CommandReservation reservation;
  reservation.year = 2027; reservation.month = 1; reservation.day = 1;
  strcpy(reservation.command, "all off");
  uint32_t id;
  assert(reservation_add(reservation, id));
  const auto before = EEPROM.bytes;
  CommandPresets presets;
  strcpy(presets.ac, "ac h 22"); strcpy(presets.hm, "hm a 53");
  assert(memory_save_presets(presets));
  assert(std::equal(before.begin(), before.begin() + kPresetsAddr, EEPROM.bytes.begin()));
  assert(std::equal(before.begin() + kPresetsAddr + sizeof(PresetsPersisted), before.end(),
                    EEPROM.bytes.begin() + kPresetsAddr + sizeof(PresetsPersisted)));
  CommandPresets loaded;
  g_initialized = false; // Reinitialize without clearing flash.
  assert(memory_load_presets(loaded));
  assert(strcmp(loaded.ac, "ac h 22") == 0 && strcmp(loaded.hm, "hm a 53") == 0);
  assert(memory_save_settings(settings));
  assert(memory_load_presets(loaded) && strcmp(loaded.hm, "hm a 53") == 0);

  EEPROM.commit_ok = false;
  strcpy(presets.ac, "ac c 26");
  assert(!memory_save_presets(presets));
  assert(memory_load_presets(loaded) && strcmp(loaded.ac, "ac h 22") == 0);
  EEPROM.commit_ok = true;
  assert(memory_save_presets(presets));

  memset(presets.ac, 'a', sizeof(presets.ac));
  assert(!memory_save_presets(presets)); // Unterminated commands cannot escape the block.
  assert(memory_load_presets(loaded) && strcmp(loaded.ac, "ac c 26") == 0);
  strcpy(presets.ac, "reboot");
  assert(!memory_save_presets(presets));

  EEPROM.bytes[kPresetsAddr] ^= 1;
  assert(!memory_load_presets(loaded) && !loaded.ac[0] && !loaded.hm[0]);
  std::fill(EEPROM.bytes.begin() + kPresetsAddr, EEPROM.bytes.end(), 0xff); // Legacy flash has no presets.
  assert(!memory_load_presets(loaded) && !loaded.ac[0] && !loaded.hm[0]);
  assert(memory_load_settings(settings));
  assert(memory_save_presets(CommandPresets()));
  CommandReservation list[RESERVATION_MAX]; size_t count;
  assert(reservation_list(list, RESERVATION_MAX, count) && count == 1 && list[0].id == id);
  assert(strcmp(list[0].command, "all off") == 0);
  assert(reservation_delete(id));
}

static CommandPresets saved_presets() {
  CommandPresets presets;
  assert(memory_load_presets(presets));
  return presets;
}

static void test_preset_commands() {
  const size_t ac_before = ac_requests.size();
  const int hm_before = hm_requests;
  const int mode_before = settings.ac_auto_mode;
  const bool auto_before = settings.hm_auto;
  run("pr n ac h 22");
  auto presets = saved_presets();
  assert(strcmp(presets.ac, "ac h 22") == 0 && !presets.hm[0]);
  run("pr n hm a 53");
  presets = saved_presets();
  assert(strcmp(presets.ac, "ac h 22") == 0 && strcmp(presets.hm, "hm a 53") == 0);
  run("pr show");
  assert(messages.back().find("AC: ac h 22") != std::string::npos);
  assert(messages.back().find("HM: hm a 53") != std::string::npos);
  run("pr n ac cool 26");
  presets = saved_presets();
  assert(strcmp(presets.ac, "ac c 26") == 0 && strcmp(presets.hm, "hm a 53") == 0);
  assert(ac_requests.size() == ac_before && hm_requests == hm_before);
  assert(settings.ac_auto_mode == mode_before && settings.hm_auto == auto_before);

  sensors.representative.humidity = 40;
  additional_sensors_request(false);
  const int before_execution = hm_requests;
  run("pr on");
  assert(ac_requests.size() == ac_before + 1 && ac.state == AC_STATE_COOL && ac.temp == 26);
  assert(settings.ac_auto_mode == AC_AUTO_OFF && settings.hm_auto && settings.hm_target_humidity == 53);
  assert(hm_requests == before_execution + 1 && device_state == HM_STRONG);

  run("pr new ac auto heat 23.5");
  presets = saved_presets();
  assert(strcmp(presets.ac, "ac a h 23.5") == 0 && strcmp(presets.hm, "hm a 53") == 0);
  assert(settings.ac_auto_mode == AC_AUTO_OFF); // Registration is separate from execution.
  run("pr on");
  assert(ac.state == AC_STATE_HEAT && ac.temp == 24);
  assert(settings.ac_auto_mode == AC_AUTO_HEAT && settings.ac_auto_temp == 23.5);
  assert(settings.hm_auto && settings.hm_target_humidity == 53);

  struct Case { const char *input; const char *ac; const char *hm; };
  for (const auto &item : {
         Case{"pr n ac d 16", "ac d 16", "hm a 53"},
         Case{"pr n ac h 30", "ac h 30", "hm a 53"},
         Case{"pr n ac h 2.2e1", "ac h 22", "hm a 53"},
         Case{"pr n ac a d 26.5", "ac a d 26.5", "hm a 53"},
         Case{"pr n ac a c 25", "ac a c 25", "hm a 53"},
         Case{"pr n hm auto 50.5", "ac a c 25", "hm a 50.5"},
         Case{"pr n hm a 0", "ac a c 25", "hm a 0"},
         Case{"pr n hm a 100", "ac a c 25", "hm a 100"},
         Case{"pr n hm on weak", "ac a c 25", "hm on w"},
         Case{"pr n hm on s", "ac a c 25", "hm on s"},
         Case{"pr n ac a off", "ac a off", "hm on s"},
         Case{"pr n hm off", "ac a off", "hm off"},
         Case{"pr n ac off", "ac off", "hm off"}}) {
    const size_t requests_before = ac_requests.size();
    const int queries_before = hm_requests;
    run(item.input);
    presets = saved_presets();
    assert(strcmp(presets.ac, item.ac) == 0 && strcmp(presets.hm, item.hm) == 0);
    assert(ac_requests.size() == requests_before && hm_requests == queries_before);
  }
  run("pr on");
  assert(ac.state == AC_STATE_OFF && device_state == HM_OFF);
  assert(settings.ac_auto_mode == AC_AUTO_OFF && !settings.hm_auto);
}

static void test_invalid_preset_commands() {
  const auto before = EEPROM.bytes;
  const size_t requests_before = ac_requests.size();
  const int queries_before = hm_requests;
  const int commits_before = EEPROM.commits;
  for (const char *invalid : {
         "pr", "pr unknown", "pr n", "pr n ac", "pr n ac h", "pr n ac h 15", "pr n ac h 31",
         "pr n ac h nan", "pr n ac h inf", "pr n ac h 22abc", "pr n ac h 22.5", "pr n ac h 22 extra",
         "pr n ac a h", "pr n ac a h 0", "pr n ac a h 31", "pr n ac a h nan", "pr n ac a h inf",
         "pr n ac a h 23 extra", "pr n ac off extra", "pr n ac a off extra",
         "pr n hm", "pr n hm 0", "pr n hm 53", "pr n hm 100",
         "pr n hm a -1", "pr n hm a 101", "pr n hm a nan", "pr n hm a inf", "pr n hm a 53abc",
         "pr n hm a", "pr n hm a 53 extra", "pr n hm a 0.0000000000000000000000000000000000001",
         "pr n hm on", "pr n hm on bad", "pr n hm off extra", "pr n hm mention on", "pr n hm alert off",
         "pr n reboot", "pr n all off", "pr n pr on", "pr on extra", "pr show extra", "pr clear extra", "pr c extra"}) {
    run(invalid);
    assert(messages.back().find("[ERROR]") == 0);
    assert(EEPROM.bytes == before);
  }
  assert(ac_requests.size() == requests_before && hm_requests == queries_before && EEPROM.commits == commits_before);
}

static void test_single_presets_clear_and_failures() {
  // Clearing presets never switches off the currently running devices.
  const auto requests_before = ac_requests.size();
  const int queries_before = hm_requests;
  run("pr clear");
  auto presets = saved_presets();
  assert(!presets.ac[0] && !presets.hm[0]);
  run("pr show");
  assert(messages.back().find("AC: (empty)") != std::string::npos && messages.back().find("HM: (empty)") != std::string::npos);
  run("pr on");
  assert(messages.back().find("No presets registered") != std::string::npos);
  assert(ac_requests.size() == requests_before && hm_requests == queries_before);

  run("pr n ac h 22");
  run("pr on");
  assert(ac.state == AC_STATE_HEAT && ac.temp == 22 && hm_requests == queries_before);
  run("pr c");
  presets = saved_presets();
  assert(!presets.ac[0] && !presets.hm[0] && ac.state == AC_STATE_HEAT);

  run("pr n hm on strong");
  run("pr on");
  assert(ac_requests.size() == requests_before + 1 && device_state == HM_STRONG && !settings.hm_auto);
  run("pr n hm on w"); run("pr on");
  assert(device_state == HM_WEAK);
  run("pr n hm a 53");
  run("pr n ac h 22");
  EEPROM.commit_ok = false;
  run("pr n hm a 60");
  assert(messages.back().find("[ERROR]") == 0);
  assert(strcmp(saved_presets().hm, "hm a 53") == 0);
  run("pr clear");
  assert(messages.back().find("[ERROR]") == 0);
  assert(strcmp(saved_presets().ac, "ac h 22") == 0 && strcmp(saved_presets().hm, "hm a 53") == 0);
  EEPROM.commit_ok = true;

  // Loss of one controller does not prevent the other preset from executing.
  device_online = false;
  run("pr on");
  assert(ac.state == AC_STATE_HEAT && settings.hm_auto && settings.hm_target_humidity == 53);
  assert(messages.back().find("waiting for controller") != std::string::npos);
  run("all off");
  assert(!settings.hm_auto && settings.ac_auto_mode == AC_AUTO_OFF);
  assert(strcmp(saved_presets().ac, "ac h 22") == 0 && strcmp(saved_presets().hm, "hm a 53") == 0);
  device_online = true;

  // Never execute a malformed saved command, even with a valid block checksum.
  PresetsPersisted malformed = {};
  EEPROM.get(kPresetsAddr, malformed);
  strcpy(malformed.commands.ac, "ac h 99");
  malformed.checksum = compute_checksum(malformed);
  EEPROM.put(kPresetsAddr, malformed);
  const auto invalid_before = ac_requests.size();
  run("pr on");
  assert(ac_requests.size() == invalid_before);
  assert(settings.hm_auto && settings.hm_target_humidity == 53);
  run("pr c");
}

static void test_reserved_preset_and_all_off() {
  now.year = 2026; now.month = 11; now.day = 12; now.hour = 13; now.minute = 0;
  run("pr n ac h 22"); run("pr n hm a 53");
  run("r n 1400 pr on"); run("r n 1410 all off");
  run("pr n ac c 26"); // Execute the latest preset when the reservation is due.
  now.hour = 14;
  command_execute_reservations(now, sensors, settings, ac, data, image);
  assert(ac.state == AC_STATE_COOL && ac.temp == 26 && settings.hm_auto && settings.hm_target_humidity == 53);
  now.minute = 10;
  command_execute_reservations(now, sensors, settings, ac, data, image);
  assert(ac.state == AC_STATE_OFF && device_state == HM_OFF);
  assert(settings.ac_auto_mode == AC_AUTO_OFF && !settings.hm_auto);
  CommandReservation list[RESERVATION_MAX]; size_t count;
  assert(reservation_list(list, RESERVATION_MAX, count) && count == 0);
}

int main() {
  test_all_off();
  test_preset_storage();
  test_preset_commands();
  test_invalid_preset_commands();
  test_single_presets_clear_and_failures();
  test_reserved_preset_and_all_off();
  std::cout << "All command tests passed.\n";
}
