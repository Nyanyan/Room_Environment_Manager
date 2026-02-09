#include "time_manager.h"
#include "sensors.h"
#include "air_conditioner.h"
#include "command.h"
#include "graph.h"

int regular_message_last_sent_hour = -1;

void regular_message(Time_info &time_info, Sensor_data &sensor_data, Settings &settings, AC_status &ac_status, Graph_data &graph_data, Graph_img &graph_img){
  if (time_info.hour != regular_message_last_sent_hour){
    regular_message_last_sent_hour = time_info.hour;
    Serial.println("[INFO] REGULAR MESSAGE");
    command_send_environment(sensor_data, settings, ac_status, graph_data, graph_img, time_info);
    time_info = time_get();
  }
}