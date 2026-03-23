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

void display_print_info(float temperature_c, float humidity_pct, float pressure_hpa, float co2_concentration_ppm) {
  lcd.clear();

  auto fmt_or_placeholder = [](float value, int decimals, const char *suffix, const char *placeholder) {
    if (value == FLT_MAX) {
      return String(placeholder);
    }
    return String(String(value, decimals) + String(suffix));
  };

  display_print(0, 0, fmt_or_placeholder(temperature_c, 1, " *C ", "---- *C  "));
  display_print(9, 0, fmt_or_placeholder(humidity_pct, 0, " %   ", "---- %  "));
  display_print(0, 1, fmt_or_placeholder(pressure_hpa, 0, " hPa", "---- hPa"));
  display_print(9, 1, fmt_or_placeholder(co2_concentration_ppm, 0, " ppm", "---- ppm"));
}