#include <Arduino.h>
#include <cassert>
#include <iostream>
#include "display.ino"

static void expect_row(int row, const char *text) {
  std::string expected(text);
  assert(expected.size() <= 20);
  expected.resize(20, ' ');
  assert(lcd.rows[row] == expected);
}

int main() {
  init_display();
  Sensor_data sensors;
  sensors.representative.temperature = 23.5f;
  sensors.representative.humidity = 50.0f;
  sensors.representative.pressure = 1012.6f;
  sensors.representative.co2_concentration = 418;
  Settings settings;
  settings.ac_auto_mode = AC_AUTO_COOL; settings.ac_auto_temp = 26.5;
  settings.hm_auto = true; settings.hm_target_humidity = 55;
  AC_status ac; ac.state = AC_STATE_COOL; ac.temp = 24;
  Time_info time{}; time.hour = 12; time.minute = 34; time.second = 56;
  display_print_info(sensors, settings, ac, time);
  expect_row(0, "23.5 *C   12:34:56");
  expect_row(1, "50.0 %    AC COOL 24");
  expect_row(2, "1013 hPa  AUTO C26.5");
  expect_row(3, "418 ppm   HM A55.0%");

  // Shrinking every kind of field must erase the old tail, including the last column.
  sensors.representative.humidity = 100;
  sensors.representative.co2_concentration = 12345;
  settings.hm_target_humidity = 100;
  display_print_info(sensors, settings, ac, time);
  expect_row(3, "12345 ppm HM A100.0%");
  sensors.representative.temperature = -5.5;
  sensors.representative.humidity = 9;
  sensors.representative.co2_concentration = 400;
  settings.ac_auto_mode = AC_AUTO_OFF; settings.hm_auto = false;
  ac.state = AC_STATE_OFF;
  display_print_info(sensors, settings, ac, time);
  expect_row(0, "-5.5 *C   12:34:56");
  expect_row(1, "9.0 %     AC OFF");
  expect_row(2, "1013 hPa  AC MANUAL");
  expect_row(3, "400 ppm   HM MANUAL");

  sensors.representative.temperature = NAN;
  sensors.representative.humidity = INFINITY;
  sensors.representative.pressure = FLT_MAX;
  sensors.representative.co2_concentration = 1e30f;
  time.hour = -1;
  display_print_info(sensors, settings, ac, time);
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
  std::cout << "LCD tests passed: 4-row layout, integer pressure, shorter fields, missing/oversized values and clipping.\n";
}
