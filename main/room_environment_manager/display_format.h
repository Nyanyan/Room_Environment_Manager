#ifndef DISPLAY_FORMAT_H
#define DISPLAY_FORMAT_H

#include <math.h>
#include <stdio.h>
#include <string.h>
#include "air_conditioner.h"
#include "command.h"
#include "sensors.h"

constexpr int DISPLAY_COLUMNS = 20;
constexpr int DISPLAY_ROWS = 4;
constexpr int DISPLAY_LEFT_WIDTH = 9;
constexpr int DISPLAY_RIGHT_X = 10;
constexpr int DISPLAY_RIGHT_WIDTH = DISPLAY_COLUMNS - DISPLAY_RIGHT_X;

struct DisplayFrame {
  char rows[DISPLAY_ROWS][DISPLAY_COLUMNS + 1];
  DisplayFrame() {
    for (int y = 0; y < DISPLAY_ROWS; ++y) {
      memset(rows[y], ' ', DISPLAY_COLUMNS);
      rows[y][DISPLAY_COLUMNS] = '\0';
    }
  }
};

// Erase the whole field and clip at its boundary. This character LCD cannot
// display Unicode: control bytes and UTF-8 bytes must not become random glyphs.
inline void display_format_field(DisplayFrame &frame, int x, int y, int width, const char *text) {
  if (x < 0 || x >= DISPLAY_COLUMNS || y < 0 || y >= DISPLAY_ROWS || width <= 0) return;
  if (width > DISPLAY_COLUMNS - x) width = DISPLAY_COLUMNS - x;
  memset(frame.rows[y] + x, ' ', width);
  if (!text) return;
  for (int i = 0; i < width && text[i]; ++i) {
    const unsigned char c = text[i];
    frame.rows[y][x + i] = c >= ' ' && c <= '~' ? (char)c : ' ';
  }
}

inline void display_format_number(char *text, size_t capacity, double value, int decimals, const char *suffix) {
  if (isfinite(value) && value != FLT_MAX) {
    const int length = snprintf(text, capacity, "%.*f%s", decimals, value, suffix);
    if (length >= 0 && (size_t)length < capacity) return;
  }
  snprintf(text, capacity, "--%s", suffix);
}

inline void display_format_clock(DisplayFrame &frame, int hour, int minute, int second) {
  char text[9] = "--:--:--";
  if (hour >= 0 && hour < 24 && minute >= 0 && minute < 60 && second >= 0 && second < 60) {
    snprintf(text, sizeof(text), "%02d:%02d:%02d", hour, minute, second);
  }
  display_format_field(frame, DISPLAY_RIGHT_X, 0, DISPLAY_RIGHT_WIDTH, text);
}

inline DisplayFrame display_format_info(const Sensor_data &sensors, const Settings &settings,
                                       const AC_status &ac) {
  DisplayFrame frame;
  const SensorReading &reading = sensors.representative;
  const float values[] = {reading.temperature, reading.humidity, reading.pressure, reading.co2_concentration};
  const char *suffixes[] = {" *C", " %", " hPa", " ppm"};
  for (int y = 0; y < DISPLAY_ROWS; ++y) {
    char field[DISPLAY_LEFT_WIDTH + 1];
    display_format_number(field, sizeof(field), values[y], y < 2 ? 1 : 0, suffixes[y]);
    display_format_field(frame, 0, y, DISPLAY_LEFT_WIDTH, field);
  }
  display_format_clock(frame, -1, -1, -1); // Filled from the live clock by the LCD task.

  char field[DISPLAY_RIGHT_WIDTH + 1];
  if (ac.state == AC_STATE_OFF) {
    snprintf(field, sizeof(field), "AC OFF");
  } else {
    const char *mode = ac.state == AC_STATE_COOL ? "COOL" :
      ac.state == AC_STATE_DRY ? "DRY" : ac.state == AC_STATE_HEAT ? "HEAT" : "?";
    char temperature[3] = "--";
    if (ac.temp >= AC_TEMP_LIMIT_MIN && ac.temp <= AC_TEMP_LIMIT_MAX) {
      snprintf(temperature, sizeof(temperature), "%d", ac.temp);
    }
    snprintf(field, sizeof(field), "AC %s %s", mode, temperature);
  }
  display_format_field(frame, DISPLAY_RIGHT_X, 1, DISPLAY_RIGHT_WIDTH, field);

  if (settings.ac_auto_mode == AC_AUTO_OFF) {
    snprintf(field, sizeof(field), "AC MANUAL");
  } else {
    const char mode = settings.ac_auto_mode == AC_AUTO_COOL ? 'C' :
      settings.ac_auto_mode == AC_AUTO_DRY ? 'D' : settings.ac_auto_mode == AC_AUTO_HEAT ? 'H' : '?';
    char target[5];
    display_format_number(target, sizeof(target), settings.ac_auto_temp, 1, "");
    snprintf(field, sizeof(field), "AUTO %c%s", mode, target);
  }
  display_format_field(frame, DISPLAY_RIGHT_X, 2, DISPLAY_RIGHT_WIDTH, field);

  if (settings.hm_auto) {
    char target[7]; // 100.0% plus terminator
    display_format_number(target, sizeof(target), settings.hm_target_humidity, 1, "%");
    snprintf(field, sizeof(field), "HM A%s", target);
  } else {
    snprintf(field, sizeof(field), "HM MANUAL");
  }
  display_format_field(frame, DISPLAY_RIGHT_X, 3, DISPLAY_RIGHT_WIDTH, field);
  return frame;
}

#endif
