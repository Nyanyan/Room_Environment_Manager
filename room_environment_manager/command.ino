#include "command.h"



struct Command command_get(){
  String slack_message = slack_get_message();
  int index1 = slack_message.indexOf(" ", 0);
  int index2 = slack_message.indexOf(" ", index1 + 1);
  Command res;
  res.cmd = slack_message.substring(0, index1);
  res.arg1 = slack_message.substring(index1 + 1, index2);
  res.arg2 = slack_message.substring(index2 + 1, slack_message.length());
  return res;
}



void command_send_environment(Sensor_data &sensor_data, Settings &settings, AC_status &ac_status, Time_info &time_info) {
  String str = "<room environment>\n";
  if (sensor_data.temperature >= 31.0){
    if (settings.alert_when_hot){
      str += "<!channel>\n";
    }
    str += "!!!!!HEAT!!!!!\n";
  }
  str += "Temp : " + String(sensor_data.temperature) + " *C\n";
  str += "Hum : " + String(sensor_data.humidity) + " pct.\n";
  str += "Pres : " + String(sensor_data.pressure) + " hPa\n";
  str += "CO2 : " + String(sensor_data.co2_concentration) + " ppm\n";
  str += "THI : " + String(sensor_calc_thi(sensor_data.temperature, sensor_data.humidity)) + "\n";
  if (settings.ac_auto_mode){
    str += "AC auto : ON\n";
  } else {
    str += "AC auto : OFF\n";
  }
  if (ac_status.state != AC_STATE_OFF){
    String ac_mode = "?";
    if (ac_status.state == AC_STATE_COOL) {
      ac_mode = "Cool";
    } else if (ac_status.state == AC_STATE_DRY) {
      ac_mode = "Dry";
    } else if (ac_status.state == AC_STATE_HEAT) {
      ac_mode = "Heat";
    }
    str += "AC status : " + ac_mode + " " + String(ac_status.temp) + " *C\n";
  } else {
    str += "AC status : OFF\n";
  }
  if (settings.alert_when_hot){
    str += "Alert when hot : ON\n";
  } else{
    str += "Alert when hot : OFF\n";
  }
  slack_send_message(time_info, str);
}



void command_send_graph(Graph_data &graph_data, Graph_img &graph_img, Time_info &time_info){
  graph_draw_temperature(graph_data, graph_img, time_info);
  graph_encode_bmp(graph_img);
  slack_upload_img(graph_img.bmp_img, BMP_GRAPH_FILE_SIZE, BMP_GRAPH_FILE_NAME_TEMPERATURE);

  graph_draw_humidity(graph_data, graph_img, time_info);
  graph_encode_bmp(graph_img);
  slack_upload_img(graph_img.bmp_img, BMP_GRAPH_FILE_SIZE, BMP_GRAPH_FILE_NAME_HUMIDITY);

  graph_draw_pressure(graph_data, graph_img, time_info);
  graph_encode_bmp(graph_img);
  slack_upload_img(graph_img.bmp_img, BMP_GRAPH_FILE_SIZE, BMP_GRAPH_FILE_NAME_PRESSURE);

  graph_draw_co2_concentration(graph_data, graph_img, time_info);
  graph_encode_bmp(graph_img);
  slack_upload_img(graph_img.bmp_img, BMP_GRAPH_FILE_SIZE, BMP_GRAPH_FILE_NAME_CO2_CONCENTRATION);

  graph_draw_thi(graph_data, graph_img, time_info);
  graph_encode_bmp(graph_img);
  slack_upload_img(graph_img.bmp_img, BMP_GRAPH_FILE_SIZE, BMP_GRAPH_FILE_NAME_THI);
}



void command_print_command_list(Time_info &time_info){
  String str = "<command list>\n";
  str += "- ac\n";
  str += "  - ac [mode] [temp]\n";
  str += "    - on air conditioner\n";
  str += "    - mode: cool (c) / dry (d) / heat (h)\n";
  str += "    - temp must be in [16,30]\n";
  str += "  - ac off\n";
  str += "    - off air conditioner\n";
  str += "  - ac auto [on/off]\n";
  str += "    - auto cooler on off\n";
  str += "- set\n";
  str += "  - set alert [on/off]\n";
  str += "    - alert on slack when very hot\n";
  str += "- monitor\n";
  str += "  - check environment\n";
  str += "- help\n";
  str += "- reboot\n";
  slack_send_message(time_info, str);
}



void command_check_ac(Command command, Time_info &time_info, Settings &settings, AC_status &ac_status){
  if (command.cmd != "ac") {
    return;
  }
  if (command.arg1 == "cool" || command.arg1 == "c"){ // ac cool [arg2]
    int set_temp = command.arg2.toInt();
    if (set_temp < AC_TEMP_LIMIT_MIN || AC_TEMP_LIMIT_MAX < set_temp){ // ac cool [error]
      String str = "[ERROR] AC COOL temp error expected [16,30] got: " + command.arg2;
      Serial.println(str);
      slack_send_message(time_info, str);
    } else{ // ac cool [temp]
      settings.ac_auto_mode = false;
      String str = "[INFO] AC COOL temp: " + command.arg2 + " *C";
      Serial.println(str);
      slack_send_message(time_info, str);
      ac_cool_on(ac_status, set_temp);
    }
  } else if (command.arg1 == "dry" || command.arg1 == "d"){ // ac dry [arg2]
    int set_temp = command.arg2.toInt();
    if (set_temp < AC_TEMP_LIMIT_MIN || AC_TEMP_LIMIT_MAX < set_temp){ // ac dry [error]
      String str = "[ERROR] AC DRY temp error expected [16,30] got: " + command.arg2;
      Serial.println(str);
      slack_send_message(time_info, str);
    } else{ // ac dry [temp]
      settings.ac_auto_mode = false;
      String str = "[INFO] AC DRY temp: " + command.arg2 + " *C";
      Serial.println(str);
      slack_send_message(time_info, str);
      ac_dry_on(ac_status, set_temp);
    }
  } else if (command.arg1 == "heat" || command.arg1 == "h"){ // ac heat [arg2]
    int set_temp = command.arg2.toInt();
    if (set_temp < AC_TEMP_LIMIT_MIN || AC_TEMP_LIMIT_MAX < set_temp){ // ac heat [error]
      String str = "[ERROR] AC HEAT temp error expected [16,30] got: " + command.arg2;
      Serial.println(str);
      slack_send_message(time_info, str);
    } else{ // ac heat [temp]
      settings.ac_auto_mode = false;
      String str = "[INFO] AC HEAT temp: " + command.arg2 + " *C";
      Serial.println(str);
      slack_send_message(time_info, str);
      ac_heat_on(ac_status, set_temp);
    }
  } else if (command.arg1 == "off"){ // ac off
    settings.ac_auto_mode = false;
    String str = "[INFO] AC OFF";
    Serial.println(str);
    slack_send_message(time_info, str);
    ac_off(ac_status);
  } else if (command.arg1 == "auto"){ // ac auto [arg2]
    if (command.arg2 == "on"){ // ac auto on
      settings.ac_auto_mode = true;
      String str = "[INFO] AC AUTO MODE ON";
      Serial.println(str);
      slack_send_message(time_info, str);
    } else if (command.arg2 == "off"){ // ac auto off
      settings.ac_auto_mode = false;
      String str = "[INFO] AC AUTO MODE OFF";
      Serial.println(str);
      slack_send_message(time_info, str);
    } else { // ac auto [error]
      String str = "[ERROR] AC AUTO WHAT???";
      Serial.println(str);
      slack_send_message(time_info, str);
    }
  } else { // ac [error]
    String str = "[ERROR] AC COMMAND NOT FOUND";
    Serial.println(str);
    slack_send_message(time_info, str);
  }
}




void command_check_set(Command command, Time_info &time_info){
  if (command.cmd != "set") {
    return;
  }
  if (command.arg1 == "alert"){ // set alert [arg2]
    if (command.arg2 == "on"){ // set alert on
      String str = "[INFO] SET ALERT ON";
      Serial.println(str);
      slack_send_message(time_info, str);
      settings.alert_when_hot = true;
    } else if (command.arg2 == "off"){ // set alert off
      String str = "[INFO] SET ALERT OFF";
      Serial.println(str);
      slack_send_message(time_info, str);
      settings.alert_when_hot = false;
    } else{ // set alert [error]
      String str = "[ERROR] SET ALERT WHAT???";
      Serial.println(str);
      slack_send_message(time_info, str);
    }
  } else{ // set [error]
    String str = "[ERROR] SET WHAT???";
    Serial.println(str);
    slack_send_message(time_info, str);
  }
}



void command_check_monitor(Command command, Time_info &time_info, Sensor_data &sensor_data, Settings &settings, AC_status &ac_status, Graph_data &graph_data, Graph_img &graph_img) {
  if (command.cmd != "monitor") {
    return;
  }
  Serial.println("[INFO] MONITOR");
  command_send_environment(sensor_data, settings, ac_status, time_info);
  command_send_graph(graph_data, graph_img, time_info);
}



void command_check_help(Command command, Time_info &time_info) {
  if (command.cmd != "help") {
    return;
  }
  Serial.println("[INFO] HELP");
  command_print_command_list(time_info);
}



void command_check_reboot(Command command, Time_info &time_info) {
  if (command.cmd != "reboot") {
    return;
  }
  String str = "[INFO] REBOOT";
  Serial.println(str);
  slack_send_message(time_info, str);
  ESP.restart();
}











