#include "time.h"
#include "time_manager.h"

struct Time_info time_get_local(){
  struct tm time;
  getLocalTime(&time);
  Time_info time_info;
  sprintf(time_info.time_str, "%02d:%02d:%02d", time.tm_hour, time.tm_min, time.tm_sec);
  time_info.hour = time.tm_hour;
  time_info.minute = time.tm_min;
  time_info.second = time.tm_sec;
  return time_info;
}

struct Time_info time_get() {
  configTime(9 * 3600, 0, ntp_server1, ntp_server2, ntp_server3);
  return time_get_local();
}
