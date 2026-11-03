#include <LiquidCrystal.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <time.h>
#include "display.h"

LiquidCrystal lcd(RS_PIN, E_PIN, DB4_PIN, DB5_PIN, DB6_PIN, DB7_PIN);

static DisplayFrame display_pending_frame;
static DisplayFrame display_rendered_frame;
static bool display_rendered_valid = false;
static bool display_clock_enabled = false;
static portMUX_TYPE display_frame_mux = portMUX_INITIALIZER_UNLOCKED;
// ESP32 FreeRTOS stack sizes are bytes. Allocate the task before network work
// without consuming fragmented heap later in the device's lifetime.
static StackType_t display_task_stack[3072];
static StaticTask_t display_task_buffer;
static TaskHandle_t display_task_handle = nullptr;

static void display_refresh() {
  DisplayFrame frame;
  bool clock_enabled;
  portENTER_CRITICAL(&display_frame_mux);
  frame = display_pending_frame;
  clock_enabled = display_clock_enabled;
  portEXIT_CRITICAL(&display_frame_mux);

  if (clock_enabled) {
    struct tm now = {};
    if (getLocalTime(&now, 0)) { // Never wait for NTP/network in the rendering task.
      display_format_clock(frame, now.tm_hour, now.tm_min, now.tm_sec);
    } else {
      display_format_clock(frame, -1, -1, -1);
    }
  }

  // Only this task touches the LCD after init. Write changed runs, including
  // trailing spaces, so there is no interleaved command/data or clear flicker.
  for (int y = 0; y < DISPLAY_ROWS; ++y) {
    int x = 0;
    while (x < DISPLAY_COLUMNS) {
      if (display_rendered_valid && frame.rows[y][x] == display_rendered_frame.rows[y][x]) {
        ++x;
        continue;
      }
      lcd.setCursor(x, y);
      do {
        lcd.write((uint8_t)frame.rows[y][x++]);
      } while (x < DISPLAY_COLUMNS &&
               (!display_rendered_valid || frame.rows[y][x] != display_rendered_frame.rows[y][x]));
    }
  }
  display_rendered_frame = frame;
  display_rendered_valid = true;
}

static void display_task(void *) {
  TickType_t wake = xTaskGetTickCount();
  for (;;) {
    display_refresh();
    vTaskDelayUntil(&wake, pdMS_TO_TICKS(1000));
  }
}

void init_display() {
  if (display_task_handle) return;
  lcd.begin(DISPLAY_COLUMNS, DISPLAY_ROWS);
  display_task_handle = xTaskCreateStaticPinnedToCore(display_task, "lcd", sizeof(display_task_stack),
    nullptr, 1, display_task_stack, &display_task_buffer, ARDUINO_RUNNING_CORE);
  configASSERT(display_task_handle);
}

void display_clear() {
  const DisplayFrame blank;
  portENTER_CRITICAL(&display_frame_mux);
  display_pending_frame = blank;
  display_clock_enabled = false;
  portEXIT_CRITICAL(&display_frame_mux);
}

void display_print(int x, int y, const char *str) {
  if (x < 0 || x >= DISPLAY_COLUMNS || y < 0 || y >= DISPLAY_ROWS) return;
  portENTER_CRITICAL(&display_frame_mux);
  display_format_field(display_pending_frame, x, y, DISPLAY_COLUMNS - x, str);
  display_clock_enabled = false;
  portEXIT_CRITICAL(&display_frame_mux);
}

void display_print(int x, int y, String str) {
  display_print(x, y, str.c_str());
}

void display_print_info(const Sensor_data &sensor_data, const Settings &settings,
                        const AC_status &ac_status) {
  const DisplayFrame frame = display_format_info(sensor_data, settings, ac_status);
  portENTER_CRITICAL(&display_frame_mux);
  display_pending_frame = frame;
  display_clock_enabled = true;
  portEXIT_CRITICAL(&display_frame_mux);
}
