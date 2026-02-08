#ifndef COMMAND_H
#define COMMAND_H

// ac auto state
#define AC_AUTO_OFF 0
#define AC_AUTO_COOL 1
#define AC_AUTO_DRY 2
#define AC_AUTO_HEAT 3

struct Command{
  String cmd;
  String arg1;
  String arg2;
  String arg3;
  String arg4;
};

struct Settings{
  bool alert_when_hot;
  int ac_auto_mode;
  double ac_auto_temp;

  Settings(){
    alert_when_hot = true;
    ac_auto_mode = AC_AUTO_OFF;
  }
};

#endif