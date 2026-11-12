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
}

int main() {
  test_all_off();
  test_preset_storage();
  std::cout << "All command tests passed.\n";
}
