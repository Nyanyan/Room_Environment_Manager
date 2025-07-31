#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <ir_Panasonic.h>
#include "air_conditioner.h"
#include "command.h"
#include "sensors.h"
#include "time_manager.h"

#define AC_N_TRY 1

// air conditioner
IRPanasonicAc ac(AC_LED_PIN);



void init_ac(){
  ac.begin();
}



void ac_cool_on(AC_status &ac_status, int set_temp){
  ac_status.state = AC_STATE_COOL;
  ac_status.temp = set_temp;
  for (int i = 0; i < AC_N_TRY; ++i){
    ac.setModel(kPanasonicRkr);
    ac.on();
    ac.setFan(kPanasonicAcFanAuto);
    ac.setMode(kPanasonicAcCool);
    ac.setTemp(set_temp);
    ac.setSwingVertical(kPanasonicAcSwingVHighest);
    ac.setSwingHorizontal(kPanasonicAcSwingHAuto);
    ac.send();
    delay(1000);
  }
}

void ac_dry_on(AC_status &ac_status, int set_temp){
  ac_status.state = AC_STATE_DRY;
  ac_status.temp = set_temp;
  for (int i = 0; i < AC_N_TRY; ++i){
    ac.setModel(kPanasonicRkr);
    ac.on();
    ac.setFan(kPanasonicAcFanAuto);
    ac.setMode(kPanasonicAcDry);
    ac.setTemp(set_temp);
    ac.setSwingVertical(kPanasonicAcSwingVHighest);
    ac.setSwingHorizontal(kPanasonicAcSwingHAuto);
    ac.send();
    delay(1000);
  }
}

void ac_heat_on(AC_status &ac_status, int set_temp){
  ac_status.state = AC_STATE_HEAT;
  ac_status.temp = set_temp;
  for (int i = 0; i < AC_N_TRY; ++i){
    ac.setModel(kPanasonicRkr);
    ac.on();
    ac.setFan(kPanasonicAcFanAuto);
    ac.setMode(kPanasonicAcHeat);
    ac.setTemp(set_temp);
    ac.setSwingVertical(kPanasonicAcSwingVHighest);
    ac.setSwingHorizontal(kPanasonicAcSwingHAuto);
    ac.send();
    delay(1000);
  }
}



void ac_off(AC_status &ac_status){
  ac_status.state = AC_STATE_OFF;
  for (int i = 0; i < 5; ++i){
    ac.setModel(kPanasonicRkr);
    ac.off();
    ac.send();
    delay(1000);
  }
}



void ac_auto(Settings &settings, Sensor_data &sensor_data, AC_status &ac_status, Time_info &time_info){
  if (settings.ac_auto_mode != AC_AUTO_OFF) {
    // hot count update
    if (sensor_data.temperature >= settings.ac_auto_temp + AC_AUTO_THRESHOLD3){
      ac_status.hot_count += 100;
    } else if (sensor_data.temperature >= settings.ac_auto_temp + AC_AUTO_THRESHOLD2){
      ac_status.hot_count += 10;
    } else if (sensor_data.temperature >= settings.ac_auto_temp + AC_AUTO_THRESHOLD1){
      ac_status.hot_count += 1;
    } else {
      ac_status.hot_count = 0;
    }
    // cold count update
    if (sensor_data.temperature <= settings.ac_auto_temp - AC_AUTO_THRESHOLD3){
      ac_status.cold_count += 100;
    } else if (sensor_data.temperature <= settings.ac_auto_temp - AC_AUTO_THRESHOLD2){
      ac_status.cold_count += 10;
    } else if (sensor_data.temperature <= settings.ac_auto_temp - AC_AUTO_THRESHOLD1){
      ac_status.cold_count += 1;
    } else {
      ac_status.cold_count = 0;
    }

    Serial.print(settings.ac_auto_temp);
    Serial.print(" ");
    Serial.print(sensor_data.temperature);
    Serial.print("  ");
    Serial.print(ac_status.hot_count);
    Serial.print(" ");
    Serial.print(ac_status.cold_count);
    Serial.print("  ");
    Serial.println(ac_status.temp);

    // update air conditioner
    if (ac_status.hot_count >= AC_AUTO_ENDURE || ac_status.cold_count >= AC_AUTO_ENDURE){ // too hot or too cold!!
      int set_temp = round(settings.ac_auto_temp); // initial temperature
      if (ac_status.hot_count >= AC_AUTO_ENDURE && ac_status.temp > AC_TEMP_LIMIT_MIN) { // stronger cool or dry / weaker heat
        set_temp = ac_status.temp - 1;
      } else if (ac_status.cold_count >= AC_AUTO_ENDURE && ac_status.temp < AC_TEMP_LIMIT_MAX) { // weaker cool or dry / stronger heat
        set_temp = ac_status.temp + 1;
      }
      String str = "[INFO] AC AUTO ON temp: " + String(set_temp) + " *C";
      Serial.println(str);
      slack_send_message(time_info, str);
      ac_status.hot_count = 0;
      ac_status.cold_count = 0;
      if (settings.ac_auto_mode == AC_AUTO_COOL) {
        ac_cool_on(ac_status, set_temp);
      } else if (settings.ac_auto_mode == AC_AUTO_DRY) {
        ac_dry_on(ac_status, set_temp);
      } else if (settings.ac_auto_mode == AC_AUTO_HEAT) {
        ac_heat_on(ac_status, set_temp);
      }
    }
  } else {
    ac_status.hot_count = 0;
    ac_status.cold_count = 0;
  }
}






