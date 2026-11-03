#include <Arduino.h>
#include <cassert>
#include <iostream>
#include "display.ino"

static void expect_row(int row, const char *text) {
  display_refresh();
  std::string expected(text);
  assert(expected.size() <= 20);
  expected.resize(20, ' ');
  assert(lcd.rows[row] == expected);
}

int main() {
  init_display();
  init_display();
  assert(fake_task_creations == 1 && fake_task_entry != nullptr);
  Sensor_data sensors;
  sensors.representative.temperature = 23.5f;
  sensors.representative.humidity = 50.0f;
  sensors.representative.pressure = 1012.6f;
  sensors.representative.co2_concentration = 418;
  Settings settings;
  settings.ac_auto_mode = AC_AUTO_COOL; settings.ac_auto_temp = 26.5;
  settings.hm_auto = true; settings.hm_target_humidity = 55;
  AC_status ac; ac.state = AC_STATE_COOL; ac.temp = 24;
  const int before_publish = lcd.writes;
  display_print_info(sensors, settings, ac);
  assert(lcd.writes == before_publish); // Main loop only queues a frame; task owns LCD writes.
  expect_row(0, "23.5 *C   12:34:56");
  expect_row(1, "50.0 %    AC COOL 24");
  expect_row(2, "1013 hPa  AUTO C26.5");
  expect_row(3, "418 ppm   HM A55.0%");

  // Shrinking every kind of field must erase the old tail, including the last column.
  sensors.representative.humidity = 100;
  sensors.representative.co2_concentration = 12345;
  settings.hm_target_humidity = 100;
  display_print_info(sensors, settings, ac);
  expect_row(3, "12345 ppm HM A100.0%");
  sensors.representative.temperature = -5.5;
  sensors.representative.humidity = 9;
  sensors.representative.co2_concentration = 400;
  settings.ac_auto_mode = AC_AUTO_OFF; settings.hm_auto = false;
  ac.state = AC_STATE_OFF;
  display_print_info(sensors, settings, ac);
  expect_row(0, "-5.5 *C   12:34:56");
  expect_row(1, "9.0 %     AC OFF");
  expect_row(2, "1013 hPa  AC MANUAL");
  expect_row(3, "400 ppm   HM MANUAL");

  sensors.representative.temperature = NAN;
  sensors.representative.humidity = INFINITY;
  sensors.representative.pressure = FLT_MAX;
  sensors.representative.co2_concentration = 1e30f;
  fake_clock_valid = false;
  display_print_info(sensors, settings, ac);
  expect_row(0, "-- *C     --:--:--");
  expect_row(1, "-- %      AC OFF");
  expect_row(2, "-- hPa    AC MANUAL");
  expect_row(3, "-- ppm    HM MANUAL");

  // Startup/error messages obey the same field boundaries and printable-byte rule.
  display_print(0, 0, "012345678901234567890123456789");
  expect_row(0, "01234567890123456789");
  display_print(0, 0, "OK");
  expect_row(0, "OK");
  display_print(18, 0, "ABCD");
  expect_row(0, "OK                AB");
  display_print(0, 1, "A\nB\tC\x01\xff");
  expect_row(1, "A B C");
  const int writes = lcd.writes;
  display_print(-1, 0, "bad"); display_print(20, 0, "bad"); display_print(0, 4, "bad");
  assert(lcd.writes == writes);

  // Run the actual task loop with no further main-loop/display_print_info calls.
  // A simulated upload blocks the main loop across midnight, but each tick draws
  // the current second while the sensor/control fields remain unchanged.
  fake_clock_valid = true;
  fake_clock_base = 23 * 3600 + 59 * 60 + 58;
  fake_clock_started_ms = fake_now;
  sensors.representative.temperature = 23.5f;
  display_print_info(sensors, settings, ac);
  std::vector<std::string> clocks;
  fake_before_tick = [&] {
    clocks.push_back(lcd.rows[0].substr(DISPLAY_RIGHT_X, 8));
    assert(lcd.rows[0].substr(0, 9) == "23.5 *C  ");
    assert(lcd.rows[2].substr(DISPLAY_RIGHT_X) == "AC MANUAL ");
  };
  fake_task_cycles = 5;
  try { fake_task_entry(nullptr); } catch (const TaskStopped &) {}
  assert((clocks == std::vector<std::string>{"23:59:58", "23:59:59", "00:00:00", "00:00:01", "00:00:02"}));
  fake_before_tick = nullptr;

  // The next tick changes only one LCD character; no clear/repaint flicker.
  const int before_second = lcd.writes;
  const int before_clear = lcd.clears;
  display_refresh();
  assert(lcd.writes - before_second == 1);
  assert(lcd.clears == before_clear);
  const int before_idle = lcd.writes;
  display_refresh();
  assert(lcd.writes == before_idle);
  assert(fake_critical_depth == 0);

  // NTP recovery and clock adjustments use live time, never a stale report timestamp.
  fake_clock_valid = false;
  expect_row(0, "23.5 *C   --:--:--");
  fake_clock_valid = true; fake_clock_base = 8 * 3600; fake_clock_started_ms = fake_now;
  expect_row(0, "23.5 *C   08:00:00");
  display_clear();
  for (int row = 0; row < 4; ++row) expect_row(row, "");
  assert(lcd.clears == before_clear);
  std::cout << "LCD tests passed: layout, bounded fields, mode changes, missing values, queued rendering, independent 1-second clock and midnight rollover.\n";
}
