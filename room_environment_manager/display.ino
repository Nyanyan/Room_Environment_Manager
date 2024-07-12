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


void display_print_info(Sensor_data &sensor_data, Settings &settings, Time_info &time_info){
  display_print(0, 0, String(sensor_data.temperature, 1) + " *C ");
  display_print(11, 0, String(sensor_data.humidity, 0) + " %  ");

  display_print(0, 1, String(sensor_data.pressure, 1) + " hPa ");
  display_print(11, 1, String(sensor_data.co2_concentration, 0) + " ppm  ");

  if (settings.ac_auto_mode){
    display_print(0, 2, "AC AUTO  ");
  } else{
    display_print(0, 2, "AC MANUAL");
  }
  if (settings.alert_when_hot){
    display_print(11, 2, "ALERT ON ");
  } else{
    display_print(11, 2, "ALERT OFF");
  }
  display_print(0, 3, String(time_info.time_str));
}