#ifndef TIME_MANAGER_H
#define TIME_MANAGER_H

#define ntp_server1 "ntp.nict.jp"
#define ntp_server2 "time.google.com"
#define ntp_server3 "ntp.jst.mfeed.ad.jp"

struct Time_info{
  char time_str[256];
  int hour;
  int minute;
  int second;
};

#endif