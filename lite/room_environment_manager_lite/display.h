#ifndef DISPLAY_H
#define DISPLAY_H

// forward declarations for display helpers
#include "air_conditioner.h"
#include "sensors.h"
#include "time_manager.h"
#include "command.h"

// display SC2004CSLB
#define RS_PIN D2
#define E_PIN D6
#define DB4_PIN D1
#define DB5_PIN D10
#define DB6_PIN D0
#define DB7_PIN D9

void init_display();
void display_clear();
void display_print(int x, int y, String str);
void display_print(int x, int y, const char *str);
void display_print_info(Sensor_data &sensor_data, Settings &settings, AC_status &ac_status, Time_info &time_info);

// pause/resume periodic LCD updates (used to avoid timing interference during IR send)
void suspend_display_updates();
void resume_display_updates();

#endif