#ifndef SLACK_REPORT_H
#define SLACK_REPORT_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <math.h>
#include "sensors.h"

// Slack's native table block, not a Markdown table inside mrkdwn text.
// https://docs.slack.dev/reference/block-kit/blocks/table-block/
inline void slack_report_cell(JsonArray row, const String &text) {
  JsonObject cell = row.add<JsonObject>();
  cell["type"] = "raw_text";
  cell["text"] = text;
}

inline String slack_report_number(float value, int decimals) {
  return value != FLT_MAX && isfinite(value) ? String(value, decimals) : String("N/A");
}

inline void slack_report_sensor_row(JsonArray rows, String &fallback, const String &label, const SensorReading &reading) {
  const String values[] = {
    slack_report_number(reading.temperature, 1), slack_report_number(reading.humidity, 1),
    slack_report_number(reading.pressure, 1), slack_report_number(reading.co2_concentration, 0)
  };
  const char *names[] = {"Temp", "Hum", "Pres", "CO2"};
  const char *units[] = {" °C", " %", " hPa", " ppm"};
  JsonArray row = rows.add<JsonArray>();
  slack_report_cell(row, label);
  fallback += label + ": ";
  for (size_t column = 0; column < 4; ++column) {
    slack_report_cell(row, values[column]);
    if (column != 0) fallback += ", ";
    fallback += String(names[column]) + " " + values[column];
    if (values[column] != "N/A") fallback += units[column];
  }
  fallback += "\n";
}

inline void slack_report_section(JsonArray blocks, const String &text) {
  if (text.length() == 0) return;
  JsonObject section = blocks.add<JsonObject>();
  section["type"] = "section";
  section["text"]["type"] = "mrkdwn";
  section["text"]["text"] = text;
}

inline void slack_build_environment_report(JsonDocument &body, const char *timestamp, const Sensor_data &sensors,
                                           const String &heading, const String &details) {
  static_assert(N_ADDITIONAL_SENSORS + 3 <= 100, "Slack tables support at most 100 rows");
  body.clear();
  JsonArray blocks = body["blocks"].to<JsonArray>();
  String fallback = String(timestamp) + "\n" + heading;
  slack_report_section(blocks, fallback);

  JsonObject table = blocks.add<JsonObject>();
  table["type"] = "table";
  JsonArray columns = table["column_settings"].to<JsonArray>();
  for (size_t column = 0; column < 5; ++column) {
    JsonObject setting = columns.add<JsonObject>();
    setting["align"] = column == 0 ? "left" : "right";
    setting["is_wrapped"] = column == 0;
  }
  JsonArray rows = table["rows"].to<JsonArray>();
  JsonArray header = rows.add<JsonArray>();
  for (const char *title : {"Sensor", "Temp (°C)", "Hum (%)", "Pres (hPa)", "CO2 (ppm)"}) {
    slack_report_cell(header, title);
  }
  slack_report_sensor_row(rows, fallback, "親", sensors.parent);
  for (int i = 0; i < N_ADDITIONAL_SENSORS; ++i) {
    slack_report_sensor_row(rows, fallback, String("子") + String(i + 1), sensors.additional[i]);
  }
  slack_report_sensor_row(rows, fallback, "代表", sensors.representative);
  slack_report_section(blocks, details);
  // Slack uses top-level text for notifications and screen readers when blocks exist.
  body["text"] = fallback + details;
}

#endif
