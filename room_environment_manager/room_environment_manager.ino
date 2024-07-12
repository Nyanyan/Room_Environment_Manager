#include "air_conditioner.h"
#include "display.h"
#include "graph.h"
#include "sensors.h"
#include "slack.h"
#include "time_manager.h"
#include "command.h"

AC_status ac_status;
Graph_data graph_data;
Graph_img graph_img;
Settings settings;

void setup() {
  Serial.begin(115200);
  init_display();
  display_print(0, 0, "[I] Initializing");
  init_wifi();
  Time_info time_info = time_get();
  slack_send_message(time_info, "[INFO] Starting...");
  init_ac();  
  init_graph(graph_img);
  init_sensors();

  display_clear();
  display_print(0, 0, "Started");
  slack_send_message(time_info, "[INFO] Started");
}



void loop() {
  Time_info time_info = time_get_local();

  // get sensor data
  Sensor_data sensor_data = get_sensor_data();

  // get command from user
  Command command = command_get();

  // check command
  command_check_ac(command, time_info, settings, ac_status);
  command_check_set(command, time_info);
  command_check_monitor(command, time_info, sensor_data, settings, ac_status, graph_data, graph_img);
  command_check_help(command, time_info);
  command_check_reboot(command, time_info);

  // air conditioner auto mode
  ac_auto(settings, sensor_data, ac_status, time_info);

  // update graph data
  graph_data_update(time_info, graph_data, sensor_data);

  // send regular message
  regular_message(time_info, sensor_data, settings, ac_status, graph_data, graph_img);
  
  // LCD
  display_print_info(sensor_data, settings, time_info);

  delay(1000);
}