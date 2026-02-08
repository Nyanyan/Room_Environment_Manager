#ifndef DISPLAY_H
#define DISPLAY_H

// forward declarations for display helpers
#include "air_conditioner.h"
#include "sensors.h"
#include "time_manager.h"
#include "command.h"

// display SC2004CSLB
#define RS_PIN 4
#define E_PIN 0
#define DB4_PIN 25
#define DB5_PIN 26
#define DB6_PIN 27
#define DB7_PIN 14

void init_display();
void display_clear();
void display_print(int x, int y, String str);
void display_print(int x, int y, const char *str);
void display_print_info(Sensor_data &sensor_data, Settings &settings, AC_status &ac_status, Time_info &time_info);

#endif