#include <esp_now.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <esp_wifi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "air_conditioner.h"
#include "display.h"
#include "graph.h"
#include "sensors.h"
#include "slack.h"
#include "time_manager.h"
#include "command.h"
#include "memory.h"
#include "humidifier.h"

AC_status ac_status;
Graph_data graph_data;
Graph_img graph_img;
Settings settings;

void setup() {
  Serial.begin(115200);
  memory_init();
  memory_load_settings(settings);
  init_display();
  display_print(0, 0, "[I] Initializing");
  init_wifi();
  Serial.print("Parent STA MAC: ");
  Serial.println(WiFi.macAddress());
  Time_info time_info = time_get();
  slack_send_message(time_info, "[INFO] Starting...");
  init_ac(ac_status);  
  init_graph(graph_img);
  init_sensors();

  display_clear();
  display_print(0, 0, "Started");
  time_info = time_get_local();
  slack_send_message(time_info, "[INFO] Started");
}



void loop() {
  slack_maintain();

  // get sensor data
  Sensor_data sensor_data = get_sensor_data();
  additional_sensors_set_representative(sensor_data.representative);
  slack_maintain();

  // update time
  Time_info time_info = time_get_local();

  // update graph data
  graph_data_update(time_info, graph_data, sensor_data, settings, ac_status);
  slack_maintain();

  // get command from user
  Command command = command_get();
  command_process(command, time_info, sensor_data, settings, ac_status, graph_data, graph_img);
  command_execute_reservations(time_info, sensor_data, settings, ac_status, graph_data, graph_img);
  slack_maintain();

  // air conditioner auto mode
  ac_auto(settings, sensor_data, ac_status, time_info);
  humidifier_update(settings, sensor_data, time_info);
  graph_data_update(time_info, graph_data, sensor_data, settings, ac_status);
  slack_maintain();

  // Publish current values before any long report/upload. The LCD task keeps
  // the clock ticking independently while this loop waits for sensors/network.
  display_print_info(sensor_data, settings, ac_status);

  // send regular message
  regular_message(time_info, sensor_data, settings, ac_status, graph_data, graph_img);
  slack_maintain();

  slack_delay(1000);
}
