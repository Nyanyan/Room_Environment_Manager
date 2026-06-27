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

// Track the latest ESP-NOW delivery status when pinging the AC controller.
static volatile bool g_ac_send_cb_called = false;
static volatile bool g_ac_send_cb_success = false;
static unsigned long g_last_ac_ping_ms = 0;
static bool g_last_ac_ping_ok = false;
static bool g_ac_reset_wifi_for_espnow = false;
constexpr unsigned long AC_PING_TIMEOUT_MS = 300;
constexpr unsigned long AC_PING_CACHE_MS = 30000; // reuse ping result for 30s to avoid WiFi flapping

static void on_ac_ping_send(const uint8_t *, esp_now_send_status_t status) {
  g_ac_send_cb_called = true;
  g_ac_send_cb_success = (status == ESP_NOW_SEND_SUCCESS);
}

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

static bool can_use_connected_wifi_for_ac_espnow() {
  return WiFi.status() == WL_CONNECTED && WiFi.channel() == CHANNEL;
}

static void reset_wifi_radio() {
  WiFi.persistent(false);
  WiFi.disconnect();
  WiFi.mode(WIFI_OFF);
  delay(10);
}

void reset_wifi() {
  slack_prepare_for_wifi_reset();
  reset_wifi_radio();
}

bool espnow_init() {
  g_ac_reset_wifi_for_espnow = !can_use_connected_wifi_for_ac_espnow();
  if (g_ac_reset_wifi_for_espnow) {
    Serial.println(String("[INFO] AC ESP-NOW needs WiFi channel ") + String(CHANNEL) +
                   String("; reconnecting WiFi after send"));
    reset_wifi();
    WiFi.mode(WIFI_STA);
    esp_wifi_set_channel(CHANNEL, WIFI_SECOND_CHAN_NONE);
  } else {
    Serial.println("[INFO] AC ESP-NOW using connected WiFi channel");
  }

  Serial.print("STA MAC: "); Serial.println(WiFi.macAddress());
  Serial.print("STA CHANNEL "); Serial.println(WiFi.channel());

  esp_err_t init_res = esp_now_init();
  if (init_res != ESP_OK) {
    Serial.printf("ESPNow Init Failed (%d)\n", init_res);
    return false;
  }
  Serial.println("ESPNow Init Success");

  memset(&slave, 0, sizeof(slave));
  for (int i = 0; i < 6; ++i) {
    slave.peer_addr[i] = slave_mac_addr[i];
  }
  slave.channel = g_ac_reset_wifi_for_espnow ? CHANNEL : 0;
  slave.ifidx = WIFI_IF_STA;
  slave.encrypt = false;

  esp_err_t addStatus = esp_now_add_peer(&slave);
  if (addStatus == ESP_OK || addStatus == ESP_ERR_ESPNOW_EXIST) {
    Serial.println("Pair success");
    return true;
  }

  Serial.printf("Cannot Pair (%d)\n", addStatus);
  return false;
}

void finish_espnow() {
  esp_now_register_send_cb(nullptr);
  esp_now_deinit();
  if (g_ac_reset_wifi_for_espnow) {
    reset_wifi_radio();
    init_wifi();
    slack_resume_after_wifi_reset();
  }
  g_ac_reset_wifi_for_espnow = false;
}

void send_ac() {
  if (!espnow_init()) {
    finish_espnow();
    return;
  }

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

  finish_espnow();
}

void ac_cool_on(AC_status &ac_status, int set_temp){
  ac_status.state = AC_STATE_COOL;
  ac_status.temp = set_temp;
  ac_status.last_set_temp_change_ms = millis();
  send_data[N_SLAVE_HEADER] = 'C';
  send_data[N_SLAVE_HEADER + 1] = 'A' + set_temp;
  send_ac();
  memory_save_ac_status(ac_status);
}

void ac_dry_on(AC_status &ac_status, int set_temp){
  ac_status.state = AC_STATE_DRY;
  ac_status.temp = set_temp;
  ac_status.last_set_temp_change_ms = millis();
  send_data[N_SLAVE_HEADER] = 'D';
  send_data[N_SLAVE_HEADER + 1] = 'A' + set_temp;
  send_ac();
  memory_save_ac_status(ac_status);
}

void ac_heat_on(AC_status &ac_status, int set_temp){
  ac_status.state = AC_STATE_HEAT;
  ac_status.temp = set_temp;
  ac_status.last_set_temp_change_ms = millis();
  send_data[N_SLAVE_HEADER] = 'H';
  send_data[N_SLAVE_HEADER + 1] = 'A' + set_temp;
  send_ac();
  memory_save_ac_status(ac_status);
}



void ac_off(AC_status &ac_status){
  ac_status.state = AC_STATE_OFF;
  ac_status.last_set_temp_change_ms = millis();
  send_data[N_SLAVE_HEADER] = 'F';
  send_data[N_SLAVE_HEADER + 1] = 'A';
  send_ac();
  memory_save_ac_status(ac_status);
}



bool ac_controller_check_connection() {
  unsigned long now = millis();
  if (g_last_ac_ping_ms != 0 && (now - g_last_ac_ping_ms) < AC_PING_CACHE_MS) {
    return g_last_ac_ping_ok;
  }

  if (!espnow_init()) {
    finish_espnow();
    g_last_ac_ping_ms = millis();
    g_last_ac_ping_ok = false;
    return false;
  }

  g_ac_send_cb_called = false;
  g_ac_send_cb_success = false;
  esp_now_register_send_cb(on_ac_ping_send);

  uint8_t ping_payload[N_SLAVE_HEADER + N_SLAVE_DATA];
  for (int i = 0; i < N_SLAVE_HEADER; ++i) {
    ping_payload[i] = slave_header[i];
  }
  ping_payload[N_SLAVE_HEADER] = 'P';
  ping_payload[N_SLAVE_HEADER + 1] = 'A';

  esp_err_t result = esp_now_send(slave.peer_addr, ping_payload, sizeof(ping_payload));
  bool ok = false;
  if (result == ESP_OK) {
    unsigned long start = millis();
    while (!g_ac_send_cb_called && (millis() - start) < AC_PING_TIMEOUT_MS) {
      if (!g_ac_reset_wifi_for_espnow) {
        slack_maintain();
      }
      delay(10);
    }
    ok = g_ac_send_cb_called && g_ac_send_cb_success;
  } else {
    Serial.printf("[WARN] esp_now_send ping failed (%d)\n", result);
  }

  finish_espnow();

  g_last_ac_ping_ms = millis();
  g_last_ac_ping_ok = ok;
  return ok;
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
  const double current_temp = sensor_data.representative.temperature;
  if (current_temp == FLT_MAX) {
    return; // skip control when no valid temperature
  }
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
    ac_status.last_pid_sample_ms = 0;
    push_history(ac_status.pid_prev_error, now);
  }

  if (ac_status.last_pid_sample_ms != 0 && (now - ac_status.last_pid_sample_ms) < AC_PID_SAMPLE_INTERVAL_MS) {
    return; // respect PID sampling cadence
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
  ac_status.last_pid_sample_ms = now;

  const double base_set_temp = target_temp; // anchor control around target instead of current setpoint to avoid getting stuck

  int set_temp = round(base_set_temp + control);
  set_temp = constrain(set_temp, AC_TEMP_LIMIT_MIN, AC_TEMP_LIMIT_MAX);

  const bool can_change_setpoint = ac_status.last_set_temp_change_ms == 0 || (now - ac_status.last_set_temp_change_ms) >= AC_SET_TEMP_MIN_INTERVAL_MS;

  if (!can_change_setpoint || set_temp == ac_status.temp) {
    return; // either rate-limited or no effective change
  }

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






