#ifndef COMMAND_H
#define COMMAND_H

struct Command{
  String cmd;
  String arg1;
  String arg2;
};

struct Settings{
  bool alert_when_hot;
  bool ac_auto_mode;

  Settings(){
    alert_when_hot = true;
    ac_auto_mode = false;
  }
};

#endif