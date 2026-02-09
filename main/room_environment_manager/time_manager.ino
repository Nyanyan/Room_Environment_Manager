#include "time.h"
#include "time_manager.h"

struct Time_info time_get_local(){
  struct tm time;
  getLocalTime(&time);
  Time_info time_info;
  time_info.year = time.tm_year + 1900;
  time_info.month = time.tm_mon + 1;
  time_info.day = time.tm_mday;
  time_info.hour = time.tm_hour;
  time_info.minute = time.tm_min;
  time_info.second = time.tm_sec;
  sprintf(time_info.day_str, "%04d/%02d/%02d", time_info.year, time_info.month, time_info.day);
  sprintf(time_info.time_str, "%02d:%02d:%02d", time_info.hour, time_info.minute, time_info.second);
  return time_info;
}

struct Time_info time_get() {
  configTime(9 * 3600, 0, ntp_server1, ntp_server2, ntp_server3);
  return time_get_local();
}
