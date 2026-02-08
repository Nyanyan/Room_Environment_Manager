// #include <IRremoteESP8266.h>
// #include <IRsend.h>
// #include <ir_Panasonic.h>
#include "air_conditioner.h"
#include "command.h"
#include "sensors.h"
#include "time_manager.h"
#include "memory.h"

// #define AC_N_TRY 1

// air conditioner
// IRPanasonicAc ac(AC_LED_PIN);



// void init_ac(){
//   ac.begin();
// }



// void ac_cool_on(AC_status &ac_status, int set_temp){
//   ac_status.state = AC_STATE_COOL;
//   ac_status.temp = set_temp;
//   for (int i = 0; i < AC_N_TRY; ++i){
//     ac.setModel(kPanasonicRkr);
//     ac.on();
//     ac.setFan(kPanasonicAcFanAuto);
//     ac.setMode(kPanasonicAcCool);
//     ac.setTemp(set_temp);
//     ac.setSwingVertical(kPanasonicAcSwingVHighest);
//     ac.setSwingHorizontal(kPanasonicAcSwingHAuto);
//     ac.send();
//     delay(1000);
//   }
// }

// void ac_dry_on(AC_status &ac_status, int set_temp){
//   ac_status.state = AC_STATE_DRY;
//   ac_status.temp = set_temp;
//   for (int i = 0; i < AC_N_TRY; ++i){
//     ac.setModel(kPanasonicRkr);
//     ac.on();
//     ac.setFan(kPanasonicAcFanAuto);
//     ac.setMode(kPanasonicAcDry);
//     ac.setTemp(set_temp);
//     ac.setSwingVertical(kPanasonicAcSwingVHighest);
//     ac.setSwingHorizontal(kPanasonicAcSwingHAuto);
//     ac.send();
//     delay(1000);
//   }
// }

// void ac_heat_on(AC_status &ac_status, int set_temp){
//   ac_status.state = AC_STATE_HEAT;
//   ac_status.temp = set_temp;
//   for (int i = 0; i < AC_N_TRY; ++i){
//     ac.setModel(kPanasonicRkr);
//     ac.on();
//     ac.setFan(kPanasonicAcFanAuto);
//     ac.setMode(kPanasonicAcHeat);
//     ac.setTemp(set_temp);
//     ac.setSwingVertical(kPanasonicAcSwingVHighest);
//     ac.setSwingHorizontal(kPanasonicAcSwingHAuto);
//     ac.send();
//     delay(1000);
//   }
// }



// void ac_off(AC_status &ac_status){
//   ac_status.state = AC_STATE_OFF;
//   for (int i = 0; i < 5; ++i){
//     ac.setModel(kPanasonicRkr);
//     ac.off();
//     ac.send();
//     delay(1000);
//   }
// }


#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include "slack.h"
#include "token.h"
esp_now_peer_info_t slave;
#define CHANNEL 1

#define N_SLAVE_DATA 2
uint8_t send_data[N_SLAVE_HEADER + N_SLAVE_DATA];

void init_ac(AC_status &ac_status){
  memory_init();
  for (int i = 0; i < N_SLAVE_HEADER; ++i) {
    send_data[i] = slave_header[i];
  }
  for (int i = 0; i < N_SLAVE_DATA; ++i) {
    send_data[N_SLAVE_HEADER + i] = 0;
  }

  if (memory_load_ac_status(ac_status)) {
    if (ac_status.state == AC_STATE_COOL && ac_status.temp >= AC_TEMP_LIMIT_MIN && ac_status.temp <= AC_TEMP_LIMIT_MAX) {
      ac_cool_on(ac_status, ac_status.temp);
    } else if (ac_status.state == AC_STATE_DRY && ac_status.temp >= AC_TEMP_LIMIT_MIN && ac_status.temp <= AC_TEMP_LIMIT_MAX) {
      ac_dry_on(ac_status, ac_status.temp);
    } else if (ac_status.state == AC_STATE_HEAT && ac_status.temp >= AC_TEMP_LIMIT_MIN && ac_status.temp <= AC_TEMP_LIMIT_MAX) {
      ac_heat_on(ac_status, ac_status.temp);
    }
  }
}

void reset_wifi() {
  WiFi.persistent(false);
  WiFi.disconnect();
  WiFi.mode(WIFI_OFF);
  delay(10);
}

void InitESPNow() {
  WiFi.disconnect();
  if (esp_now_init() == ESP_OK) {
    Serial.println("ESPNow Init Success");
  }
  else {
    Serial.println("ESPNow Init Failed");
    // Retry InitESPNow, add a counte and then restart?
    // InitESPNow();
    // or Simply Restart
    ESP.restart();
  }
}

void espnow_init() {
  //Set device in STA mode to begin with
  WiFi.mode(WIFI_STA);
  esp_wifi_set_channel(CHANNEL, WIFI_SECOND_CHAN_NONE);
  // This is the mac address of the Master in Station Mode
  Serial.print("STA MAC: "); Serial.println(WiFi.macAddress());
  Serial.print("STA CHANNEL "); Serial.println(WiFi.channel());
  // Init ESPNow with a fallback logic
  InitESPNow();
  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Trasnmitted packet


  // const uint8_t slave_mac_addr[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  memset(&slave, 0, sizeof(slave));
  for (int i = 0; i < 6; ++i) {
    slave.peer_addr[i] = slave_mac_addr[i];
  }
  while (true){
    esp_err_t addStatus = esp_now_add_peer(&slave);
    if (addStatus == ESP_OK) {
      // Pair success
      Serial.println("Pair success");
      break;
    } else{
      Serial.println("Cannot Pair");
    }
  }
}

void send_ac() {
  reset_wifi();
  espnow_init();
  esp_err_t result = esp_now_send(slave.peer_addr, send_data, N_SLAVE_HEADER + N_SLAVE_DATA);
  if (result == ESP_OK) {
    // data_status = STATUS_SEND_SUCCESS;
    //Serial.println("Success");
  } else {
    // data_status = STATUS_SEND_FAILED;
    if (result == ESP_ERR_ESPNOW_NOT_INIT) {
      // How did we get so far!!
      Serial.println("ESPNOW not Init.");
    } else if (result == ESP_ERR_ESPNOW_ARG) {
      Serial.println("Invalid Argument");
    } else if (result == ESP_ERR_ESPNOW_INTERNAL) {
      Serial.println("Internal Error");
    } else if (result == ESP_ERR_ESPNOW_NO_MEM) {
      Serial.println("ESP_ERR_ESPNOW_NO_MEM");
    } else if (result == ESP_ERR_ESPNOW_NOT_FOUND) {
      Serial.println("Peer not found.");
    } else {
      Serial.println("Not sure what happened");
    }
  }
  reset_wifi();
  init_wifi();
}

void ac_cool_on(AC_status &ac_status, int set_temp){
  ac_status.state = AC_STATE_COOL;
  ac_status.temp = set_temp;
  send_data[N_SLAVE_HEADER] = 'C';
  send_data[N_SLAVE_HEADER + 1] = 'A' + set_temp;
  send_ac();
  memory_save_ac_status(ac_status);
}

void ac_dry_on(AC_status &ac_status, int set_temp){
  ac_status.state = AC_STATE_DRY;
  ac_status.temp = set_temp;
  send_data[N_SLAVE_HEADER] = 'D';
  send_data[N_SLAVE_HEADER + 1] = 'A' + set_temp;
  send_ac();
  memory_save_ac_status(ac_status);
}

void ac_heat_on(AC_status &ac_status, int set_temp){
  ac_status.state = AC_STATE_HEAT;
  ac_status.temp = set_temp;
  send_data[N_SLAVE_HEADER] = 'H';
  send_data[N_SLAVE_HEADER + 1] = 'A' + set_temp;
  send_ac();
  memory_save_ac_status(ac_status);
}



void ac_off(AC_status &ac_status){
  ac_status.state = AC_STATE_OFF;
  send_data[N_SLAVE_HEADER] = 'F';
  send_data[N_SLAVE_HEADER + 1] = 'A';
  send_ac();
  memory_save_ac_status(ac_status);
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
      int set_temp = ac_status.temp; // initial temperature
      if (ac_status.hot_count >= AC_AUTO_ENDURE && ac_status.temp > AC_TEMP_LIMIT_MIN) { // stronger cool or dry / weaker heat
        set_temp = ac_status.temp - 1;
      } else if (ac_status.cold_count >= AC_AUTO_ENDURE && ac_status.temp < AC_TEMP_LIMIT_MAX) { // weaker cool or dry / stronger heat
        set_temp = ac_status.temp + 1;
      }
      ac_status.hot_count = 0;
      ac_status.cold_count = 0;
      if (ac_status.temp != set_temp) {
        // String str = "[INFO] AC AUTO ON temp: " + String(set_temp) + " *C";
        // Serial.println(str);
        // slack_send_message(time_info, str);
        if (settings.ac_auto_mode == AC_AUTO_COOL) {
          ac_cool_on(ac_status, set_temp);
        } else if (settings.ac_auto_mode == AC_AUTO_DRY) {
          ac_dry_on(ac_status, set_temp);
        } else if (settings.ac_auto_mode == AC_AUTO_HEAT) {
          ac_heat_on(ac_status, set_temp);
        }
      }
    }
  } else {
    ac_status.hot_count = 0;
    ac_status.cold_count = 0;
  }
}






