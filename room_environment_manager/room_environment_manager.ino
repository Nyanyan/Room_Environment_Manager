#include "air_conditioner.h"
#include "display.h"
#include "graph.h"
#include "sensors.h"
#include "slack.h"
#include "time_manager.h"
#include "command.h"


#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h> // only for esp_wifi_set_channel()
// Global copy of slave
esp_now_peer_info_t slave;
#define CHANNEL 1
#define PRINTSCANRESULTS 0
#define DELETEBEFOREPAIR 0
uint8_t send_data[3] = {'A', 'K', 0};

// Init ESP Now with fallback
void InitESPNow() {
  WiFi.disconnect();
  if (esp_now_init() == ESP_OK) {
    Serial.println("ESPNow Init Success");
  }
  else {
    Serial.println("ESPNow Init Failed");
    // Retry InitESPNow, add a counte and then restart?
    // InitESPNow();
    // or Simply Restart
    ESP.restart();
  }
}

void send_ac() {
  esp_err_t result = esp_now_send(slave.peer_addr, send_data, 3);
  if (result == ESP_OK) {
    // data_status = STATUS_SEND_SUCCESS;
    //Serial.println("Success");
  } else {
    // data_status = STATUS_SEND_FAILED;
    if (result == ESP_ERR_ESPNOW_NOT_INIT) {
      // How did we get so far!!
      Serial.println("ESPNOW not Init.");
    } else if (result == ESP_ERR_ESPNOW_ARG) {
      Serial.println("Invalid Argument");
    } else if (result == ESP_ERR_ESPNOW_INTERNAL) {
      Serial.println("Internal Error");
    } else if (result == ESP_ERR_ESPNOW_NO_MEM) {
      Serial.println("ESP_ERR_ESPNOW_NO_MEM");
    } else if (result == ESP_ERR_ESPNOW_NOT_FOUND) {
      Serial.println("Peer not found.");
    } else {
      Serial.println("Not sure what happened");
    }
  }
}


AC_status ac_status;
Graph_data graph_data;
Graph_img graph_img;
Settings settings;

void setup() {
  Serial.begin(115200);
  // init_display();
  // display_print(0, 0, "[I] Initializing");
  // init_wifi();
  // Time_info time_info = time_get();
  // slack_send_message(time_info, "[INFO] Starting...");
  // init_ac();  
  // init_graph(graph_img);
  // init_sensors();

  // display_clear();
  // display_print(0, 0, "Started");
  // time_info = time_get_local();
  // slack_send_message(time_info, "[INFO] Started");




  //Set device in STA mode to begin with
  WiFi.mode(WIFI_STA);
  esp_wifi_set_channel(CHANNEL, WIFI_SECOND_CHAN_NONE);
  // This is the mac address of the Master in Station Mode
  Serial.print("STA MAC: "); Serial.println(WiFi.macAddress());
  Serial.print("STA CHANNEL "); Serial.println(WiFi.channel());
  // Init ESPNow with a fallback logic
  InitESPNow();
  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Trasnmitted packet


  const uint8_t slave_mac_addr[6] = {0xEC, 0xDA, 0x3B, 0xBC, 0xE0, 0x79};
  memset(&slave, 0, sizeof(slave));
  for (int i = 0; i < 6; ++i) {
    slave.peer_addr[i] = slave_mac_addr[i];
  }
  while (true){
    esp_err_t addStatus = esp_now_add_peer(&slave);
    if (addStatus == ESP_OK) {
      // Pair success
      Serial.println("Pair success");
      break;
    } else{
      Serial.println("Cannot Pair");
    }
  }
}



void loop() {
  // // get sensor data
  // Sensor_data sensor_data = get_sensor_data();

  // // update time
  // Time_info time_info = time_get_local();

  // // update graph data
  // if (graph_data.last_data_update_minute == -1){ // forced update
  //   int tmp_minute = time_info.minute;
  //   time_info.minute = time_info.minute / GRAPH_DATA_INTERVAL * GRAPH_DATA_INTERVAL;
  //   graph_data_update(time_info, graph_data, sensor_data);
  //   time_info.minute = tmp_minute;
  // } else { // regular update
  //   graph_data_update(time_info, graph_data, sensor_data);
  // }

  // // get command from user
  // Command command = command_get();

  // // check command
  // command_check_ac(command, time_info, settings, ac_status);
  // command_check_set(command, time_info);
  // command_check_monitor(command, time_info, sensor_data, settings, ac_status, graph_data, graph_img);
  // command_check_help(command, time_info);
  // command_check_reboot(command, time_info);

  // // air conditioner auto mode
  // ac_cool_auto(settings, sensor_data, ac_status, time_info);

  // // send regular message
  // regular_message(time_info, sensor_data, settings, ac_status, graph_data, graph_img);

  // exhibition demo
  send_data[2] = 'i';
  send_ac();
  Serial.println("AC Open");
  delay(5000);
  send_data[2] = 'o';
  send_ac();
  Serial.println("AC Close");
  delay(5000);
  
  // // LCD
  // display_print_info(sensor_data, settings, ac_status, time_info);

  // delay(1000);
}