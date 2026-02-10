#include <LiquidCrystal.h>
#include "display.h"
#include "sensors.h"

LiquidCrystal lcd(RS_PIN, E_PIN, DB4_PIN, DB5_PIN, DB6_PIN, DB7_PIN);


void init_display() {
  lcd.begin(20, 4);
}

void display_clear(){
  lcd.clear();
}

void display_print(int x, int y, String str) {
  lcd.setCursor(x, y);
  lcd.print(str);
}

void display_print(int x, int y, const char *str) {
  lcd.setCursor(x, y);
  lcd.print(str);
}


void display_print_info(Sensor_data &sensor_data, Settings &settings, AC_status &ac_status){
  Time_info time_info = time_get_local();
  display_clear();
  const SensorReading &parent = sensor_data.parent;
  auto fmt_or_placeholder = [](float value, int decimals, const char *suffix, const char *placeholder) {
    if (value == FLT_MAX) {
      return String(placeholder);
    }
    return String(String(value, decimals) + String(suffix));
  };

  // 1st line
  display_print(0, 0, fmt_or_placeholder(parent.temperature, 1, " *C ", "---- *C "));
  display_print(11, 0, String(time_info.time_str));
  
  // 2nd line
  display_print(0, 1, fmt_or_placeholder(parent.humidity, 0, " %  ", "---- %  "));
  display_print(11, 1, fmt_or_placeholder(parent.pressure, 0, " hPa ", "---- hPa"));
  

  // 3rd line
  display_print(0, 2, "\xb4\xb1\xba\xdd"" ""\xbf\xb3\xbb"":"); // ｴｱｺﾝ ｿｳｻ:
  if (settings.ac_auto_mode == AC_AUTO_OFF){
    display_print(11, 2, "\xbc\xad\xc4\xde\xb3"); // ｼｭﾄﾞｳ
  } else {
    String ac_auto_str = "\xb5\xb0\xc4"" "; // ｵｰﾄ 
    if (settings.ac_auto_mode == AC_AUTO_COOL) {
      ac_auto_str += "C ";
    } else if (settings.ac_auto_mode == AC_AUTO_DRY) {
      ac_auto_str += "D ";
    } else if (settings.ac_auto_mode == AC_AUTO_HEAT) {
      ac_auto_str += "H ";
    }
    ac_auto_str += String((int)settings.ac_auto_temp) + "." + String((int)(settings.ac_auto_temp * 10) - ((int)settings.ac_auto_temp * 10));
    display_print(11, 2, ac_auto_str);
  }

  // 4th line
  display_print(0, 3, "\xbc\xde\xae\xb3\xc0\xb2"":"); // ｼﾞｮｳﾀｲ:
  if (ac_status.state != AC_STATE_OFF){
    String ac_mode = "?";
    if (ac_status.state == AC_STATE_COOL) {
      ac_mode = "\xda\xb2\xce\xde\xb3"; // ﾚｲﾎﾞｳ
    } else if (ac_status.state == AC_STATE_DRY) {
      ac_mode = "\xc4\xde\xd7\xb2"; // ﾄﾞﾗｲ
    } else if (ac_status.state == AC_STATE_HEAT) {
      ac_mode = "\xc0\xde\xdd\xce\xde\xb3"; // ﾀﾞﾝﾎﾞｳ
    }
    display_print(11, 3, String(ac_mode + " ") + String(ac_status.temp));
  } else{
    display_print(11, 3, "OFF");
  }
}