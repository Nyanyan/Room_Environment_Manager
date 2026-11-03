#ifndef COMMAND_H
#define COMMAND_H

// ac auto state
#define AC_AUTO_OFF 0
#define AC_AUTO_COOL 1
#define AC_AUTO_DRY 2
#define AC_AUTO_HEAT 3

#define ALERT_HOT_TEMPERATURE_DEFAULT 31.0
#define ALERT_HOT_TEMPERATURE_MIN 10.0
#define ALERT_HOT_TEMPERATURE_MAX 45.0

struct Command{
  String cmd;
  String arg1;
  String arg2;
  String arg3;
  String arg4;
};

struct Settings{
  bool alert_when_hot;
  double alert_hot_temperature;
  int ac_auto_mode;
  double ac_auto_temp;
  bool hm_auto;
  double hm_target_humidity;
  bool hm_water_alert;
  bool hm_water_mention;

  Settings(){
    alert_when_hot = true;
    alert_hot_temperature = ALERT_HOT_TEMPERATURE_DEFAULT;
    ac_auto_mode = AC_AUTO_OFF;
    ac_auto_temp = 0.0;
    hm_auto = false;
    hm_target_humidity = 50.0;
    hm_water_alert = true;
    hm_water_mention = false;
  }
};

#endif
