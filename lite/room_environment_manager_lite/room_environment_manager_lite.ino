// controller: ESP32C3

#include "air_conditioner.h"
#include "display.h"
#include "graph.h"
#include "sensors.h"
#include "slack.h"
#include "time_manager.h"
#include "command.h"
#include "memory.h"
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

AC_status ac_status;
Graph_data graph_data;
Graph_img graph_img;
Settings settings;
Sensor_data sensor_data;

hw_timer_t *display_timer = nullptr;
portMUX_TYPE display_timer_mux = portMUX_INITIALIZER_UNLOCKED;
SemaphoreHandle_t display_sem = nullptr;
TaskHandle_t display_task_handle = nullptr;

void suspend_display_updates() {
  portENTER_CRITICAL(&display_timer_mux);
  if (display_timer != nullptr) {
    // timerAlarmDisable(display_timer);
    timerStop(display_timer);
  }
  portEXIT_CRITICAL(&display_timer_mux);
  if (display_sem != nullptr) {
    xSemaphoreTake(display_sem, 0); // clear pending signals to avoid burst updates on resume
  }
}

void resume_display_updates() {
  portENTER_CRITICAL(&display_timer_mux);
  if (display_timer != nullptr) {
    timerWrite(display_timer, 0);   // restart counter so we do not fire immediately
    // timerAlarmEnable(display_timer);
    timerStart(display_timer);
  }
  portEXIT_CRITICAL(&display_timer_mux);
}

void IRAM_ATTR on_display_timer() {
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  if (display_sem != nullptr) {
    xSemaphoreGiveFromISR(display_sem, &xHigherPriorityTaskWoken);
    if (xHigherPriorityTaskWoken == pdTRUE) {
      portYIELD_FROM_ISR();
    }
  }
}

void display_task(void *pvParameters) {
  for (;;) {
    if (xSemaphoreTake(display_sem, portMAX_DELAY) == pdTRUE) {
      display_print_info(sensor_data, settings, ac_status);
    }
  }
}

void setup() {
  Serial.begin(115200);
  memory_init();
  memory_load_settings(settings);
  init_display();
  display_print(0, 0, "[I] Initializing");
  init_wifi();
  Time_info time_info = time_get();
  slack_send_message(time_info, "[INFO] 起動中...");
  init_ac(ac_status);  
  init_graph(graph_img);
  init_sensors();
  sensor_data = get_sensor_data();

  display_clear();
  display_print(0, 0, "Started");
  display_print(0, 1, "please wait a minute");
  time_info = time_get_local();
  slack_send_message(time_info, "[INFO] 起動しました");

  display_sem = xSemaphoreCreateBinary();
  xTaskCreatePinnedToCore(display_task, "display_task", 4096, nullptr, 2, &display_task_handle, 0);

  // 100ms display update timer
  display_timer = timerBegin(1000000); // 1MHz (1us per tick)
  timerAttachInterrupt(display_timer, &on_display_timer);
  timerAlarm(display_timer, 100000, true, 0); // 100ms, autoreload
  timerStart(display_timer);
  // display_timer = timerBegin(0, 80, true); // 80MHz / 80 = 1MHz (1us per tick)
  // timerAttachInterrupt(display_timer, &on_display_timer, true);
  // timerAlarmWrite(display_timer, 100000, true); // 100ms
  // timerAlarmEnable(display_timer);
}



void loop() {
  static unsigned long last_main_update_ms = 0;
  unsigned long now_ms = millis();

  if (now_ms - last_main_update_ms >= 1000) {
    last_main_update_ms = now_ms;

    // get sensor data
    sensor_data = get_sensor_data();

    // update time
    Time_info time_info = time_get_local();

    // update graph data
    if (graph_data.last_data_update_minute == -1){ // forced update
      int tmp_minute = time_info.minute;
      time_info.minute = time_info.minute / GRAPH_DATA_INTERVAL * GRAPH_DATA_INTERVAL;
      graph_data_update(time_info, graph_data, sensor_data);
      time_info.minute = tmp_minute;
    } else { // regular update
      graph_data_update(time_info, graph_data, sensor_data);
    }

    // get command from user
    Command command = command_get();
    command_process(command, time_info, sensor_data, settings, ac_status, graph_data, graph_img);
    command_execute_reservations(time_info, sensor_data, settings, ac_status, graph_data, graph_img);

    // air conditioner auto mode
    ac_auto(settings, sensor_data, ac_status, time_info);

    // send regular message
    regular_message(time_info, sensor_data, settings, ac_status, graph_data, graph_img);
  }
}