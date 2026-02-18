#include <float.h>
#include "command.h"
#include "memory.h"
#include "sensors.h"

namespace {

Command parse_command_text(const String &text) {
  Command res;
  res.cmd = "";
  res.arg1 = "";
  res.arg2 = "";
  res.arg3 = "";
  res.arg4 = "";
  if (text.length() == 0) {
    return res;
  }

  int index1 = text.indexOf(' ');
  if (index1 < 0) {
    res.cmd = text;
    return res;
  }

  res.cmd = text.substring(0, index1);
  int index2 = text.indexOf(' ', index1 + 1);
  if (index2 < 0) {
    res.arg1 = text.substring(index1 + 1);
    return res;
  }

  res.arg1 = text.substring(index1 + 1, index2);
  int index3 = text.indexOf(' ', index2 + 1);
  if (index3 < 0) {
    res.arg2 = text.substring(index2 + 1);
    return res;
  }

  res.arg2 = text.substring(index2 + 1, index3);
  int index4 = text.indexOf(' ', index3 + 1);
  if (index4 < 0) {
    res.arg3 = text.substring(index3 + 1);
    return res;
  }

  res.arg3 = text.substring(index3 + 1, index4);
  res.arg4 = text.substring(index4 + 1);
  return res;
}

bool parse_date_str(const String &date_str, uint16_t &year, uint8_t &month, uint8_t &day) {
  if (date_str.length() != 8) {
    return false;
  }
  String year_str = date_str.substring(0, 4);
  String month_str = date_str.substring(4, 6);
  String day_str = date_str.substring(6, 8);
  year = year_str.toInt();
  month = month_str.toInt();
  day = day_str.toInt();
  if (year < 2000 || month < 1 || month > 12 || day < 1 || day > 31) {
    return false;
  }
  return true;
}

bool parse_time_str(const String &time_str, uint8_t &hour, uint8_t &minute) {
  if (time_str.length() != 4) {
    return false;
  }
  hour = time_str.substring(0, 2).toInt();
  minute = time_str.substring(2, 4).toInt();
  if (hour > 23 || minute > 59) {
    return false;
  }
  return true;
}

String format_reservation_line(const CommandReservation &res) {
  char buf[128];
  snprintf(buf, sizeof(buf), "[%lu] %04u/%02u/%02u %02u:%02u %s", static_cast<unsigned long>(res.id),
           static_cast<unsigned int>(res.year), static_cast<unsigned int>(res.month), static_cast<unsigned int>(res.day),
           static_cast<unsigned int>(res.hour), static_cast<unsigned int>(res.minute), res.command);
  return String(buf);
}

void trim_leading_spaces(String &text) {
  while (text.startsWith(" ")) {
    text.remove(0, 1);
  }
}

bool reservation_before(const CommandReservation &a, const CommandReservation &b) {
  if (a.year != b.year) return a.year < b.year;
  if (a.month != b.month) return a.month < b.month;
  if (a.day != b.day) return a.day < b.day;
  if (a.hour != b.hour) return a.hour < b.hour;
  if (a.minute != b.minute) return a.minute < b.minute;
  return a.id < b.id;
}

bool is_valid_sensor_value(float v) {
  return v != FLT_MAX;
}

String format_sensor_value(float v, int decimals, const char *unit) {
  if (!is_valid_sensor_value(v)) {
    return String("N/A");
  }
  return String(v, decimals) + String(" ") + String(unit);
}

void append_sensor_block(String &str, const SensorReading &reading) {
  str += "- 温度 : " + format_sensor_value(reading.temperature, 1, "*C") + "\n";
  str += "- 湿度 : " + format_sensor_value(reading.humidity, 1, "pct.") + "\n";
  str += "- 気圧 : " + format_sensor_value(reading.pressure, 1, "hPa") + "\n";
}

} // namespace


void command_send_reservation_list(Time_info &time_info) {
  CommandReservation reservations[RESERVATION_MAX];
  size_t count = 0;
  reservation_list(reservations, RESERVATION_MAX, count);

  size_t n = count;
  if (n > RESERVATION_MAX) {
    n = RESERVATION_MAX;
  }
  for (size_t i = 0; i + 1 < n; ++i) {
    for (size_t j = i + 1; j < n; ++j) {
      if (!reservation_before(reservations[i], reservations[j])) {
        CommandReservation tmp = reservations[i];
        reservations[i] = reservations[j];
        reservations[j] = tmp;
      }
    }
  }

  String str = "<reservation list>\n";
  if (count == 0) {
    str += "(empty)\n";
  } else {
    size_t to_show = n;
    for (size_t i = 0; i < to_show; ++i) {
      str += "- " + format_reservation_line(reservations[i]) + "\n";
    }
  }
  slack_send_message(time_info, str);
  }


struct Command command_get(){
  String slack_message = slack_get_message();
  return parse_command_text(slack_message);
}



void command_send_environment(Sensor_data &sensor_data, Settings &settings, AC_status &ac_status, Graph_data &graph_data, Graph_img &graph_img, Time_info &time_info) {
  String str = "< *部屋の環境* >\n";

  append_sensor_block(str, sensor_data.parent);

  str += "< *エアコンの状態* >\n";
  str += "- エアコン自動制御: ";
  if (settings.ac_auto_mode == AC_AUTO_OFF) {
    str += "OFF\n";
  } else {
    if (settings.ac_auto_mode == AC_AUTO_COOL){
      str += "冷房 ";
    } else if (settings.ac_auto_mode == AC_AUTO_DRY){
      str += "ドライ ";
    } else if (settings.ac_auto_mode == AC_AUTO_HEAT){
      str += "暖房 ";
    }
    str += String(settings.ac_auto_temp) + " *C\n";
  }
  if (ac_status.state != AC_STATE_OFF){
    String ac_mode = "?";
    if (ac_status.state == AC_STATE_COOL) {
      ac_mode = "冷房";
    } else if (ac_status.state == AC_STATE_DRY) {
      ac_mode = "ドライ";
    } else if (ac_status.state == AC_STATE_HEAT) {
      ac_mode = "暖房";
    }
    str += "- エアコン状態: " + ac_mode + " " + String(ac_status.temp) + " *C\n";
  } else {
    str += "- エアコン状態: OFF\n";
  }
  // str += get_graph_urls(graph_data, graph_img, time_info);
  slack_send_message(time_info, str);
}



String get_graph_urls(Graph_data &graph_data, Graph_img &graph_img, Time_info &time_info){
  graph_draw_temperature(graph_data, graph_img, time_info);
  graph_encode_bmp(graph_img);
  String graph_temperature = slack_upload_img(graph_img.bmp_img, BMP_GRAPH_FILE_SIZE, BMP_GRAPH_FILE_NAME_TEMPERATURE);

  graph_draw_humidity(graph_data, graph_img, time_info);
  graph_encode_bmp(graph_img);
  String graph_humidity = slack_upload_img(graph_img.bmp_img, BMP_GRAPH_FILE_SIZE, BMP_GRAPH_FILE_NAME_HUMIDITY);

  graph_draw_pressure(graph_data, graph_img, time_info);
  graph_encode_bmp(graph_img);
  String graph_pressure = slack_upload_img(graph_img.bmp_img, BMP_GRAPH_FILE_SIZE, BMP_GRAPH_FILE_NAME_PRESSURE);

  String str = 
    String("<") + graph_temperature + String("| >") + 
    String("<") + graph_humidity + String("| >") + 
    String("<") + graph_pressure + String("| >");
  return str;
}



void command_print_command_list(Time_info &time_info){
  String str = "< *使えるコマンド* >\n";
  str += "- `ac`\n";
  str += "  - `ac [モード] [温度]`\n";
  str += "    - エアコンを起動する\n";
  str += "    - モード: `cool` (`c`) / `dry` (`d`) / `heat` (`h`)\n";
  str += "    - 設定温度は" + String(AC_TEMP_LIMIT_MIN) + "度から" + String(AC_TEMP_LIMIT_MAX) + "度まで\n";
  str += "    - 例: `ac c 28`: 冷房を28度設定でつける\n";
  str += "  - `ac auto [モード] [温度]` (`auto`は`a`と省略可能)\n";
  str += "    - 部屋の温度を一定に保つようにエアコンを制御する\n";
  str += "    - モード: `cool` (`c`) / `dry` (`d`) / `heat` (`h`)\n";
  str += "    - 例: `ac a c 28`: 部屋の温度が28度になるようにエアコンを制御する\n";
  str += "  - `ac off`\n";
  str += "    - エアコンを切る\n";
  str += "    - エアコン自動制御も切れる\n";
  str += "- `monitor`\n";
  str += "  - 部屋の環境やエアコンの設定を見る\n";
  str += "- `help`\n";
  str += "  - これを表示する\n";
  str += "- `reboot`\n";
  str += "  - 再起動する\n";
  slack_send_message(time_info, str);
}


void command_check_ac(Command command, Time_info &time_info, Settings &settings, AC_status &ac_status){
  if (command.cmd != "ac") {
    return;
  }
  if (command.arg1 == "cool" || command.arg1 == "c"){ // ac cool [arg2]
    int set_temp = command.arg2.toInt();
    if (set_temp < AC_TEMP_LIMIT_MIN || AC_TEMP_LIMIT_MAX < set_temp){ // ac cool [error]
      String str = "[ERROR] 設定温度は" + String(AC_TEMP_LIMIT_MIN) + "度から" + String(AC_TEMP_LIMIT_MAX) + "度にしてください。取得した温度: " + command.arg2;
      Serial.println(str);
      slack_send_message(time_info, str);
    } else{ // ac cool [temp]
      settings.ac_auto_mode = AC_AUTO_OFF;
      memory_save_settings(settings);
      String str = "[INFO] 冷房を設定温度" + command.arg2 + " *Cでつけました";
      Serial.println(str);
      slack_send_message(time_info, str);
      ac_cool_on(ac_status, set_temp);
    }
  } else if (command.arg1 == "dry" || command.arg1 == "d"){ // ac dry [arg2]
    int set_temp = command.arg2.toInt();
    if (set_temp < AC_TEMP_LIMIT_MIN || AC_TEMP_LIMIT_MAX < set_temp){ // ac dry [error]
      String str = "[ERROR] 設定温度は" + String(AC_TEMP_LIMIT_MIN) + "度から" + String(AC_TEMP_LIMIT_MAX) + "度にしてください。取得した温度: " + command.arg2;
      Serial.println(str);
      slack_send_message(time_info, str);
    } else { // ac dry [temp]
      settings.ac_auto_mode = AC_AUTO_OFF;
      memory_save_settings(settings);
      String str = "[INFO] ドライを設定温度" + command.arg2 + " *Cでつけました";
      Serial.println(str);
      slack_send_message(time_info, str);
      ac_dry_on(ac_status, set_temp);
    }
  } else if (command.arg1 == "heat" || command.arg1 == "h") { // ac heat [arg2]
    int set_temp = command.arg2.toInt();
    if (set_temp < AC_TEMP_LIMIT_MIN || AC_TEMP_LIMIT_MAX < set_temp){ // ac heat [error]
      String str = "[ERROR] 設定温度は" + String(AC_TEMP_LIMIT_MIN) + "度から" + String(AC_TEMP_LIMIT_MAX) + "度にしてください。取得した温度: " + command.arg2;
      Serial.println(str);
      slack_send_message(time_info, str);
    } else { // ac heat [temp]
      settings.ac_auto_mode = AC_AUTO_OFF;
      memory_save_settings(settings);
      String str = "[INFO] 暖房を設定温度" + command.arg2 + " *Cでつけました";
      Serial.println(str);
      slack_send_message(time_info, str);
      ac_heat_on(ac_status, set_temp);
    }
  } else if (command.arg1 == "off") { // ac off
    settings.ac_auto_mode = AC_AUTO_OFF;
    memory_save_settings(settings);
    String str = "[INFO] エアコンを切りました";
    Serial.println(str);
    slack_send_message(time_info, str);
    ac_off(ac_status);
  } else if (command.arg1 == "auto" || command.arg1 == "a") { // ac auto [arg2] [arg3]
    if (command.arg2 == "cool" || command.arg2 == "c") { // ac auto cool [temp]
      double set_temp = command.arg3.toDouble();
      if (set_temp == 0.0) {
        String str = "[ERROR] 設定温度を取得できませんでした。取得したもの: " + command.arg3;
        Serial.println(str);
        slack_send_message(time_info, str);
      }
      settings.ac_auto_mode = AC_AUTO_COOL;
      settings.ac_auto_temp = set_temp;
      memory_save_settings(settings);
      String str = "[INFO] 部屋の温度が" + command.arg3 + " *Cとなるように冷房を制御します";
      Serial.println(str);
      slack_send_message(time_info, str);
      ac_cool_on(ac_status, round(set_temp));
    } else if (command.arg2 == "dry" || command.arg2 == "d") { // ac auto dry [temp]
      double set_temp = command.arg3.toDouble();
      if (set_temp == 0.0) {
        String str = "[ERROR] 設定温度を取得できませんでした。取得したもの: " + command.arg3;
        Serial.println(str);
        slack_send_message(time_info, str);
      }
      settings.ac_auto_mode = AC_AUTO_DRY;
      settings.ac_auto_temp = set_temp;
      memory_save_settings(settings);
      String str = "[INFO] 部屋の温度が" + command.arg3 + " *Cとなるようにドライを制御します";
      Serial.println(str);
      slack_send_message(time_info, str);
      ac_dry_on(ac_status, round(set_temp));
    } else if (command.arg2 == "heat" || command.arg2 == "h") { // ac auto heat [temp]
      double set_temp = command.arg3.toDouble();
      if (set_temp == 0.0) {
        String str = "[ERROR] 設定温度を取得できませんでした。取得したもの: " + command.arg3;
        Serial.println(str);
        slack_send_message(time_info, str);
      }
      settings.ac_auto_mode = AC_AUTO_HEAT;
      settings.ac_auto_temp = set_temp;
      memory_save_settings(settings);
      String str = "[INFO] 部屋の温度が" + command.arg3 + " *Cとなるように暖房を制御します";
      Serial.println(str);
      slack_send_message(time_info, str);
      ac_heat_on(ac_status, round(set_temp));
    } else { // ac auto [error]
      String str = "[ERROR] 構文が違います。`ac auto [モード] [温度]`と入力してください。helpと打つと詳細を確認できます。";
      Serial.println(str);
      slack_send_message(time_info, str);
    }
  } else { // ac [error]
    String str = "[ERROR] 構文が違います。helpと打つと詳細を確認できます。";
    Serial.println(str);
    slack_send_message(time_info, str);
  }
}


void command_check_reserve(Command command, Time_info &time_info, Settings &settings, AC_status &ac_status) {
  if (command.cmd != "reserve" && command.cmd != "r") {
    return;
  }
  if (command.arg1 == "new" || command.arg1 == "n") {
    uint16_t year;
    uint8_t month;
    uint8_t day;
    if (!parse_date_str(command.arg2, year, month, day)) {
      String str = String("[ERROR] invalid date: ") + command.arg2;
      slack_send_message(time_info, str);
      return;
    }
    String time_str;
    String cmd_text;

    if (command.arg4.length() > 0) {
      time_str = command.arg3;
      cmd_text = command.arg4;
    } else {
      // Backward compatibility: arg3 contains both time and command
      String time_and_cmd = command.arg3;
      trim_leading_spaces(time_and_cmd);
      int split = time_and_cmd.indexOf(' ');
      if (split < 0) {
        slack_send_message(time_info, "[ERROR] expected time and command after date");
        return;
      }
      time_str = time_and_cmd.substring(0, split);
      cmd_text = time_and_cmd.substring(split + 1);
      trim_leading_spaces(cmd_text);
    }

    uint8_t hour;
    uint8_t minute;
    if (!parse_time_str(time_str, hour, minute)) {
      String str = String("[ERROR] invalid time: ") + time_str;
      slack_send_message(time_info, str);
      return;
    }
    if (cmd_text.length() == 0) {
      slack_send_message(time_info, "[ERROR] command is empty");
      return;
    }

    CommandReservation reservation;
    reservation.year = year;
    reservation.month = month;
    reservation.day = day;
    reservation.hour = hour;
    reservation.minute = minute;
    strncpy(reservation.command, cmd_text.c_str(), RESERVATION_COMMAND_MAX_LEN - 1);
    reservation.command[RESERVATION_COMMAND_MAX_LEN - 1] = '\0';

    uint32_t assigned_id = 0;
    if (!reservation_add(reservation, assigned_id)) {
      slack_send_message(time_info, "[ERROR] reservation storage is full");
      return;
    }

    char buf[160];
    snprintf(buf, sizeof(buf), "[INFO] reserved id:%lu %04u/%02u/%02u %02u:%02u %s",
             static_cast<unsigned long>(assigned_id), static_cast<unsigned int>(year), static_cast<unsigned int>(month),
             static_cast<unsigned int>(day), static_cast<unsigned int>(hour), static_cast<unsigned int>(minute),
             reservation.command);
    slack_send_message(time_info, String(buf));
    command_send_reservation_list(time_info);
  } else if (command.arg1 == "check" || command.arg1 == "c") {
    command_send_reservation_list(time_info);
  } else if (command.arg1 == "delete" || command.arg1 == "d") {
    if (command.arg2.length() == 0) {
      slack_send_message(time_info, "[ERROR] reservation id is missing");
      return;
    }
    uint32_t id = command.arg2.toInt();
    if (id == 0) {
      String str = String("[ERROR] invalid reservation id: ") + command.arg2;
      slack_send_message(time_info, str);
      return;
    }
    if (reservation_delete(id)) {
      String str = String("[INFO] deleted reservation id: ") + String(id);
      slack_send_message(time_info, str);
      command_send_reservation_list(time_info);
    } else {
      String str = String("[ERROR] reservation not found: ") + String(id);
      slack_send_message(time_info, str);
    }
  } else {
    slack_send_message(time_info, "[ERROR] reserve sub command not found");
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
      memory_save_settings(settings);
    } else if (command.arg2 == "off"){ // set alert off
      String str = "[INFO] SET ALERT OFF";
      Serial.println(str);
      slack_send_message(time_info, str);
      settings.alert_when_hot = false;
      memory_save_settings(settings);
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
  command_send_environment(sensor_data, settings, ac_status, graph_data, graph_img, time_info);
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
  String str = "[INFO] 再起動します";
  Serial.println(str);
  slack_send_message(time_info, str);
  ESP.restart();
}


void command_process(Command command, Time_info &time_info, Sensor_data &sensor_data, Settings &settings, AC_status &ac_status, Graph_data &graph_data, Graph_img &graph_img) {
  if (command.cmd.length() == 0) {
    return;
  }
  command_check_ac(command, time_info, settings, ac_status);
  // command_check_reserve(command, time_info, settings, ac_status);
  // command_check_set(command, time_info);
  command_check_monitor(command, time_info, sensor_data, settings, ac_status, graph_data, graph_img);
  command_check_help(command, time_info);
  command_check_reboot(command, time_info);
}


void command_execute_reservations(Time_info &time_info, Sensor_data &sensor_data, Settings &settings, AC_status &ac_status, Graph_data &graph_data, Graph_img &graph_img) {
  CommandReservation reservation;
  while (reservation_pop_due(time_info, reservation)) {
    String cmd_text = String(reservation.command);
    Command cmd = parse_command_text(cmd_text);
    String msg = String("[INFO] execute reservation id: ") + String(reservation.id) + String(" -> ") + cmd_text;
    slack_send_message(time_info, msg);
    command_process(cmd, time_info, sensor_data, settings, ac_status, graph_data, graph_img);
  }
}











