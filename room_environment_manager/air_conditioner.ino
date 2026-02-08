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
  if (settings.ac_auto_mode == AC_AUTO_OFF) {
    ac_status.hot_count = 0;
    ac_status.cold_count = 0;
    ac_status.pid_integral = 0.0;
    ac_status.pid_prev_error = 0.0;
    ac_status.pid_prev_millis = 0;
    ac_status.pid_initialized = false;
    ac_status.pid_history_size = 0;
    ac_status.pid_history_index = 0;
    return;
  }

  // Clamp target to the allowed AC range
  const double target_temp = constrain(settings.ac_auto_temp, (double)AC_TEMP_LIMIT_MIN, (double)AC_TEMP_LIMIT_MAX);
  const double current_temp = sensor_data.temperature;
  const unsigned long now = millis();

   // helper: push error/time into history ring buffer
  auto push_history = [&](double err, unsigned long ts) {
    ac_status.pid_error_history[ac_status.pid_history_index] = err;
    ac_status.pid_time_history[ac_status.pid_history_index] = ts;
    ac_status.pid_history_index = (ac_status.pid_history_index + 1) % AC_PID_HISTORY;
    if (ac_status.pid_history_size < AC_PID_HISTORY) {
      ac_status.pid_history_size++;
    }
  };

  if (!ac_status.pid_initialized) {
    ac_status.pid_prev_millis = now;
    ac_status.pid_prev_error = target_temp - current_temp;
    ac_status.pid_integral = 0.0;
    ac_status.pid_initialized = true;
    push_history(ac_status.pid_prev_error, now);
  }

  const double dt = (now - ac_status.pid_prev_millis) / 1000.0;
  if (dt <= 0.0) {
    return; // insufficient time elapsed for a stable derivative term
  }

  const double error = target_temp - current_temp;
  ac_status.pid_integral += error * dt;
  ac_status.pid_integral = constrain(ac_status.pid_integral, -AC_PID_I_CLAMP, AC_PID_I_CLAMP);
  push_history(error, now);

  // derivative using a windowed slope for noise resistance
  double derivative = 0.0;
  if (ac_status.pid_history_size >= 2) {
    int lookback = ac_status.pid_history_size < 6 ? ac_status.pid_history_size : 6; // up to last 5 intervals
    int idx_tail = (ac_status.pid_history_index - lookback + AC_PID_HISTORY) % AC_PID_HISTORY;
    double err_tail = ac_status.pid_error_history[idx_tail];
    unsigned long ts_tail = ac_status.pid_time_history[idx_tail];
    double dt_tail = (now - ts_tail) / 1000.0;
    if (dt_tail > 0.0) {
      derivative = (error - err_tail) / dt_tail;
    }
  }

  const double control = AC_PID_KP * error + AC_PID_KI * ac_status.pid_integral + AC_PID_KD * derivative;

  ac_status.pid_prev_error = error;
  ac_status.pid_prev_millis = now;

  int base_set_temp = ac_status.temp;
  if (base_set_temp < AC_TEMP_LIMIT_MIN || base_set_temp > AC_TEMP_LIMIT_MAX) {
    base_set_temp = round(target_temp);
  }

  int set_temp = round(base_set_temp + control);
  set_temp = constrain(set_temp, AC_TEMP_LIMIT_MIN, AC_TEMP_LIMIT_MAX);

  if (set_temp != ac_status.temp) {
    // String str = "[INFO] AC AUTO PID temp: " + String(set_temp) + " *C";
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






