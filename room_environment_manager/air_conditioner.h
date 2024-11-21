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

// cool auto
#define AC_COOL_AUTO_HOT_THRESHOLD1 31.0 // weight = 1
#define AC_COOL_AUTO_HOT_THRESHOLD2 32.0 // weight = 10
#define AC_COOL_AUTO_HOT_THRESHOLD3 33.0 // weight = 100
#define AC_COOL_AUTO_COLD_THRESHOLD 25.0
#define AC_COOL_AUTO_ENDURE 150 // about 30 minutes
#define AC_COOL_AUTO_TEMP 29
#define AC_COOL_AUTO_TEMP_MIN 26



struct AC_status{
  int state;
  int temp;
  int hot_count;
  int cold_count;

  AC_status(){
    state = AC_STATE_OFF;
    temp = -1;
    hot_count = 0;
    cold_count = 0;
  }
};

#endif