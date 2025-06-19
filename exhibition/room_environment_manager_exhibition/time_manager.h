#ifndef TIME_MANAGER_H
#define TIME_MANAGER_H

#define ntp_server1 "ntp.nict.jp"
#define ntp_server2 "time.google.com"
#define ntp_server3 "ntp.jst.mfeed.ad.jp"

struct Time_info{
  char day_str[256];
  char time_str[256];
  int year;
  int month;
  int day;
  int hour;
  int minute;
  int second;
};

#endif