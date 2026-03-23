#include "display.h"

void init_lcd() {
    lcd.begin(16, 2); // columns and rows
    lcd.setContrast(40);
    lcd.print("Starting...");
}

void display_print(int x, int y, String str) {
  lcd.setCursor(x, y);
  lcd.print(str);
}

void display_print_info() {
  lcd.clear();

  auto fmt_or_placeholder = [](float value, int decimals, const char *suffix, const char *placeholder) {
    if (value == FLT_MAX) {
      return String(placeholder);
    }
    return String(String(value, decimals) + String(suffix));
  };

  display_print(0, 0, fmt_or_placeholder(representative.temperature, 1, " *C ", "---- *C  "));
  display_print(9, 0, fmt_or_placeholder(representative.humidity, 0, " %   ", "---- %  "));
  display_print(0, 1, fmt_or_placeholder(representative.pressure, 0, " hPa", "---- hPa"));
  display_print(9, 1, fmt_or_placeholder(representative.co2_concentration, 0, " ppm", "---- ppm"));
}