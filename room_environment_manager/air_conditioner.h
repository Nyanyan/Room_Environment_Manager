#ifndef AIR_CONDIRIONER_H
#define AIR_CONDIRIONER_H

// air conditioner auto controller
#define AC_LED_PIN 15

// state
#define AC_STATE_OFF 0
#define AC_STATE_COOL 1
#define AC_STATE_DRY 2
#define AC_STATE_HEAT 3

// limit
#define AC_TEMP_LIMIT_MIN 16
#define AC_TEMP_LIMIT_MAX 30

// auto
#define AC_AUTO_THRESHOLD1 0.5 // weight = 1
#define AC_AUTO_THRESHOLD2 1.2 // weight = 10
#define AC_AUTO_THRESHOLD3 2.5 // weight = 100
#define AC_AUTO_ENDURE 50 // about 10 minutes

// PID constants (tuned for ~10s sensor interval)
#define AC_PID_KP 1.0
#define AC_PID_KI 0.003
#define AC_PID_KD 0.05
#define AC_PID_I_CLAMP 30.0



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

  AC_status(){
    state = AC_STATE_OFF;
    temp = -1;
    hot_count = 0;
    cold_count = 0;
    pid_integral = 0.0;
    pid_prev_error = 0.0;
    pid_prev_millis = 0;
    pid_initialized = false;
  }
};

#endif