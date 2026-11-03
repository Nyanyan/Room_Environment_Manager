#include <LiquidCrystal.h>
#include "display.h"

LiquidCrystal lcd(RS_PIN, E_PIN, DB4_PIN, DB5_PIN, DB6_PIN, DB7_PIN);

void init_display() {
  lcd.begin(DISPLAY_COLUMNS, DISPLAY_ROWS);
}

void display_clear() {
  lcd.clear();
}

void display_print(int x, int y, const char *str) {
  if (x < 0 || x >= DISPLAY_COLUMNS || y < 0 || y >= DISPLAY_ROWS) return;
  DisplayFrame frame;
  display_format_field(frame, x, y, DISPLAY_COLUMNS - x, str);
  lcd.setCursor(x, y);
  lcd.print(frame.rows[y] + x);
}

void display_print(int x, int y, String str) {
  display_print(x, y, str.c_str());
}

void display_print_info(const Sensor_data &sensor_data, const Settings &settings,
                        const AC_status &ac_status, const Time_info &time_info) {
  const DisplayFrame frame = display_format_info(sensor_data, settings, ac_status, time_info);
  for (int y = 0; y < DISPLAY_ROWS; ++y) {
    lcd.setCursor(0, y);
    lcd.print(frame.rows[y]);
  }
}
