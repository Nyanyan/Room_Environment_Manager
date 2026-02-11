#include <IRremoteESP8266.h>
#include <IRsend.h>
#define AC_USE_MITSUBISHI true // set to 0 for Panasonic

#if AC_USE_MITSUBISHI
#include <ir_Mitsubishi.h>
#else
#include <ir_Panasonic.h>
#endif
#include "air_conditioner.h"
#include "command.h"
#include "display.h"
#include "sensors.h"
#include "time_manager.h"
#include "memory.h"

#define AC_N_TRY 1

#if AC_USE_MITSUBISHI
// IR transmitter controls Mitsubishi A/C
IRMitsubishiAC ac(AC_LED_PIN);
#else
// IR transmitter controls Panasonic A/C
IRPanasonicAc ac(AC_LED_PIN);
#endif

// Guard IR transmission from timer-driven display updates to keep waveforms stable
static void ac_send_with_guard() {
  suspend_display_updates();
  ac.send();
  resume_display_updates();
  delay(1000);
}

void init_ac(AC_status &ac_status){
  ac.begin();
  memory_init();

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

void ac_cool_on(AC_status &ac_status, int set_temp){
  ac_status.state = AC_STATE_COOL;
  ac_status.temp = set_temp;
  for (int i = 0; i < AC_N_TRY; ++i){
    #if AC_USE_MITSUBISHI
    ac.on();
    ac.setFan(kMitsubishiAcFanAuto);
    ac.setMode(kMitsubishiAcCool);
    ac.setTemp(set_temp);
    ac.setVane(kMitsubishiAcVaneAuto);      // vertical auto
    ac.setVaneLeft(kMitsubishiAcVaneAuto);  // keep left vane in sync
    ac.setWideVane(kMitsubishiAcWideVaneAuto);
    #else
    ac.setModel(kPanasonicRkr);
    ac.on();
    ac.setFan(kPanasonicAcFanAuto);
    ac.setMode(kPanasonicAcCool);
    ac.setTemp(set_temp);
    ac.setSwingVertical(kPanasonicAcSwingVHighest);
    ac.setSwingHorizontal(kPanasonicAcSwingHAuto);
    #endif
    ac_send_with_guard();
  }
  memory_save_ac_status(ac_status);
}

void ac_dry_on(AC_status &ac_status, int set_temp){
  ac_status.state = AC_STATE_DRY;
  ac_status.temp = set_temp;
  for (int i = 0; i < AC_N_TRY; ++i){
    #if AC_USE_MITSUBISHI
    ac.on();
    ac.setFan(kMitsubishiAcFanAuto);
    ac.setMode(kMitsubishiAcDry);
    ac.setTemp(set_temp);
    ac.setVane(kMitsubishiAcVaneAuto);
    ac.setVaneLeft(kMitsubishiAcVaneAuto);
    ac.setWideVane(kMitsubishiAcWideVaneAuto);
    #else
    ac.setModel(kPanasonicRkr);
    ac.on();
    ac.setFan(kPanasonicAcFanAuto);
    ac.setMode(kPanasonicAcDry);
    ac.setTemp(set_temp);
    ac.setSwingVertical(kPanasonicAcSwingVHighest);
    ac.setSwingHorizontal(kPanasonicAcSwingHAuto);
    #endif
    ac_send_with_guard();
  }
  memory_save_ac_status(ac_status);
}

void ac_heat_on(AC_status &ac_status, int set_temp){
  ac_status.state = AC_STATE_HEAT;
  ac_status.temp = set_temp;
  for (int i = 0; i < AC_N_TRY; ++i){
    #if AC_USE_MITSUBISHI
    ac.on();
    ac.setFan(kMitsubishiAcFanAuto);
    ac.setMode(kMitsubishiAcHeat);
    ac.setTemp(set_temp);
    ac.setVane(kMitsubishiAcVaneAuto);
    ac.setVaneLeft(kMitsubishiAcVaneAuto);
    ac.setWideVane(kMitsubishiAcWideVaneAuto);
    #else
    ac.setModel(kPanasonicRkr);
    ac.on();
    ac.setFan(kPanasonicAcFanAuto);
    ac.setMode(kPanasonicAcHeat);
    ac.setTemp(set_temp);
    ac.setSwingVertical(kPanasonicAcSwingVHighest);
    ac.setSwingHorizontal(kPanasonicAcSwingHAuto);
    #endif
    ac_send_with_guard();
  }
  memory_save_ac_status(ac_status);
}



void ac_off(AC_status &ac_status){
  ac_status.state = AC_STATE_OFF;
  for (int i = 0; i < AC_N_TRY; ++i){
    #if AC_USE_MITSUBISHI
    ac.off();
    ac.setFan(kMitsubishiAcFanAuto);
    ac.setMode(kMitsubishiAcHeat);
    ac.setTemp(20);
    ac.setVane(kMitsubishiAcVaneAuto);
    ac.setVaneLeft(kMitsubishiAcVaneAuto);
    ac.setWideVane(kMitsubishiAcWideVaneAuto);
    #else
    ac.setModel(kPanasonicRkr);
    ac.off();
    ac.setFan(kPanasonicAcFanHigh);
    ac.setMode(kPanasonicAcCool);
    ac.setTemp(20);
    ac.setSwingVertical(kPanasonicAcSwingVAuto);
    ac.setSwingHorizontal(kPanasonicAcSwingHAuto);
    #endif
    ac_send_with_guard();
  }
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
  const double current_temp = sensor_data.parent.temperature;
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






