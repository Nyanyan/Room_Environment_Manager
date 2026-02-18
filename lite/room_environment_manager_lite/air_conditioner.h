#ifndef AIR_CONDIRIONER_H
#define AIR_CONDIRIONER_H

// air conditioner auto controller
#define AC_LED_PIN D7

// state
#define AC_STATE_OFF 0
#define AC_STATE_COOL 1
#define AC_STATE_DRY 2
#define AC_STATE_HEAT 3

// rate limiting
#define AC_SET_TEMP_MIN_INTERVAL_MS 60000UL // limit AC setpoint changes to once every minute
#define AC_PID_SAMPLE_INTERVAL_MS   30000UL // PID sampling cadence (30 seconds)

// auto
#define AC_AUTO_THRESHOLD1 0.5 // weight = 1
#define AC_AUTO_THRESHOLD2 1.2 // weight = 10
#define AC_AUTO_THRESHOLD3 2.5 // weight = 100
#define AC_AUTO_ENDURE 50 // about 10 minutes

// PID constants
#define AC_PID_KP 0.5
#define AC_PID_KI 0.05
#define AC_PID_KD 0.4
#define AC_PID_I_CLAMP 30.0

// PID history buffer
#define AC_PID_HISTORY 120



struct AC_status{
  int state; // off / cool / dry / heat
  int temp; // set temperature
  int hot_count;
  int cold_count;

  // PID bookkeeping
  double pid_integral;
  double pid_prev_error;
  unsigned long pid_prev_millis;
  bool pid_initialized;

  unsigned long last_pid_sample_ms;
  unsigned long last_set_temp_change_ms;

  // history for better smoothing/diagnostics
  double pid_error_history[AC_PID_HISTORY];
  unsigned long pid_time_history[AC_PID_HISTORY];
  int pid_history_size;
  int pid_history_index;

  AC_status(){
    state = AC_STATE_OFF;
    temp = -1;
    hot_count = 0;
    cold_count = 0;
    pid_integral = 0.0;
    pid_prev_error = 0.0;
    pid_prev_millis = 0;
    pid_initialized = false;
    pid_history_size = 0;
    pid_history_index = 0;
    last_pid_sample_ms = 0;
    last_set_temp_change_ms = 0;
  }
};

#endif