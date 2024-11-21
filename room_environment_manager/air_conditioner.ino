#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <ir_Panasonic.h>
#include "air_conditioner.h"
#include "command.h"
#include "sensors.h"
#include "time_manager.h"

// air conditioner
IRPanasonicAc ac(AC_LED_PIN);



void init_ac(){
  ac.begin();
}



void ac_cool_on(AC_status &ac_status, int set_temp){
  ac_status.state = AC_STATE_COOL;
  ac_status.temp = set_temp;
  for (int i = 0; i < 5; ++i){
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
  for (int i = 0; i < 5; ++i){
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
  for (int i = 0; i < 5; ++i){
    ac.setModel(kPanasonicRkr);
    ac.on();
    ac.setFan(kPanasonicAcFanAuto);
    ac.setMode(kPanasonicAcHeat);
    ac.setTemp(set_temp);
    ac.setSwingVertical(kPanasonicAcSwingVAuto);
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



void ac_cool_auto(Settings &settings, Sensor_data &sensor_data, AC_status &ac_status, Time_info &time_info){
  if (settings.ac_auto_mode){
    if (sensor_data.temperature >= AC_COOL_AUTO_HOT_THRESHOLD3){
      ac_status.hot_count += 100;
    } else if (sensor_data.temperature >= AC_COOL_AUTO_HOT_THRESHOLD2){
      ac_status.hot_count += 10;
    } else if (sensor_data.temperature >= AC_COOL_AUTO_HOT_THRESHOLD1){
      ac_status.hot_count += 1;
    } else {
      ac_status.hot_count = 0;
    }
    if (sensor_data.temperature <= AC_COOL_AUTO_COLD_THRESHOLD){
      ++ac_status.cold_count;
    } else{
      ac_status.cold_count = 0;
    }
    if (ac_status.hot_count >= AC_COOL_AUTO_ENDURE){ // hot!!
      int set_temp = AC_COOL_AUTO_TEMP;
      if (ac_status.state == AC_STATE_COOL && ac_status.temp > AC_COOL_AUTO_TEMP_MIN){ // stronger
        set_temp = ac_status.temp - 1;
      }
      String str = "[INFO] AC AUTO ON temp: " + String(set_temp) + " *C";
      Serial.println(str);
      slack_send_message(time_info, str);
      ac_status.hot_count = 0;
      ac_status.cold_count = 0;
      ac_cool_on(ac_status, set_temp);
    } else if (ac_status.cold_count >= AC_COOL_AUTO_ENDURE && ac_status.state == AC_STATE_COOL){ // cold!!
      String str = "[INFO] AC AUTO OFF";
      Serial.println(str);
      slack_send_message(time_info, str);
      ac_status.hot_count = 0;
      ac_status.cold_count = 0;
      ac_off(ac_status);
    }
  } else{
    ac_status.hot_count = 0;
    ac_status.cold_count = 0;
  }
}






