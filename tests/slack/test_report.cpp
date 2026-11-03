#include <cassert>
#include <iostream>
#include "slack_report.h"
#include "time_manager.h"

#define SLACK_CHANNEL_ID "C_TEST"
#define SLACK_TOKEN "test-token"
#define SLACK_URL_SEND "https://slack.com/api/chat.postMessage"
String posted_body;
String posted_url;
bool wifi_connected = true;
int post_status = 200;
class HTTPClient {
 public:
  void addHeader(const char *, const char *) {}
  int POST(const String &body) { posted_body = body; return post_status; }
  String getString() { return "{\"ok\":true,\"ts\":\"123.456\"}"; }
  String errorToString(int) { return "test error"; }
  void end() {}
};
bool ensure_wifi_connected_and_settled() { return wifi_connected; }
bool begin_http_with_retries(HTTPClient &, const char *url) { posted_url = url; return true; }
void add_slack_auth_header(HTTPClient &, const char *) {}
bool is_http_success(int status) { return status == 200; }
// Real HTTP payload construction and send wrappers, with only the network replaced.
#include "message_under_test.inc"

std::string cell(JsonDocument &body, size_t row, size_t column) {
  return body["blocks"][1]["rows"][row][column]["text"] | "";
}

int main() {
  Sensor_data sensors;
  sensors.parent.temperature = 27.02;
  sensors.parent.humidity = 42.18;
  sensors.parent.pressure = 1021.57;
  sensors.parent.co2_concentration = 418;
  sensors.additional[0].temperature = 25;
  sensors.additional[0].humidity = 48.29;
  sensors.additional[1].temperature = 21.64;
  sensors.additional[1].humidity = 50.10;
  sensors.additional[1].pressure = 1022.71;
  sensors.additional[2].temperature = 23.52;
  sensors.additional[2].humidity = 52.87;
  sensors.representative.temperature = 23.5;
  sensors.representative.humidity = 50.15;
  sensors.representative.pressure = 1022.14;
  sensors.representative.co2_concentration = 418;
  Time_info time = {};
  strcpy(time.time_str, "12:34:56");
  const String heading = "< *Room Environment* >\n<!channel>\n!!!!!HEAT!!!!!\n";
  const String details = "< *Air Conditioner* >\n- AC status : OFF\n"
                         "< *Humidifier* >\n- Status : WEAK\n- Needs water : NO\n"
                         "< *Connection* >\n- Humidifier controller : OK\n"
                         "<https://example.com/graph| >";
  assert(strcmp(slack_send_environment_report(time, sensors, heading, details), "123.456") == 0);
  assert(posted_url == SLACK_URL_SEND);
  JsonDocument body;
  assert(!deserializeJson(body, posted_body));
  assert(strcmp(body["channel"], SLACK_CHANNEL_ID) == 0);
  JsonArray blocks = body["blocks"];
  assert(blocks.size() == 3);
  assert(strcmp(blocks[0]["type"], "section") == 0);
  assert(strcmp(blocks[0]["text"]["type"], "mrkdwn") == 0);
  assert(std::string(blocks[0]["text"]["text"] | "") == std::string("12:34:56\n") + heading.c_str());
  assert(strcmp(blocks[1]["type"], "table") == 0);
  JsonArray rows = blocks[1]["rows"];
  assert(rows.size() == N_ADDITIONAL_SENSORS + 3);
  for (JsonArray row : rows) {
    assert(row.size() == 5);
    for (JsonObject value : row) {
      assert(strcmp(value["type"], "raw_text") == 0);
      assert(value["text"].is<const char *>());
    }
  }
  assert(cell(body, 0, 0) == "Sensor" && cell(body, 0, 1) == "Temp (°C)");
  assert(cell(body, 0, 2) == "Hum (%)" && cell(body, 0, 3) == "Pres (hPa)" && cell(body, 0, 4) == "CO2 (ppm)");
  assert(cell(body, 1, 0) == "parent" && cell(body, 1, 1) == "27.0" && cell(body, 1, 2) == "42.2");
  assert(cell(body, 1, 3) == "1021.6" && cell(body, 1, 4) == "418");
  assert(cell(body, 2, 0) == "additional 1" && cell(body, 2, 3) == "N/A");
  assert(cell(body, 3, 0) == "additional 2" && cell(body, 3, 3) == "1022.7");
  assert(cell(body, 4, 0) == "additional 3" && cell(body, 4, 4) == "N/A");
  assert(cell(body, 5, 0) == "representative" && cell(body, 5, 1) == "23.5");
  assert(blocks[1]["column_settings"].size() == 5);
  for (size_t column = 0; column < 5; ++column) {
    assert(strcmp(blocks[1]["column_settings"][column]["align"], column == 0 ? "left" : "right") == 0);
  }
  assert(std::string(blocks[2]["text"]["text"] | "") == details.c_str());
  const std::string fallback = body["text"] | "";
  assert(fallback.find("Hum 42.2 %") != std::string::npos);
  assert(fallback.find("additional 3:") != std::string::npos);
  assert(fallback.find("representative:") != std::string::npos);
  assert(fallback.find("<!channel>") != std::string::npos);
  assert(fallback.find(details.c_str()) != std::string::npos);
  assert(posted_body.find("pct.") == std::string::npos);

  // Missing, NaN and infinite sensor values remain legible, and the report
  // serializes even when every optional sensor is absent.
  sensors = Sensor_data();
  sensors.parent.humidity = NAN;
  sensors.parent.pressure = INFINITY;
  sensors.parent.temperature = -5.21;
  slack_send_environment_report(time, sensors, "Room Environment\n", details);
  assert(!deserializeJson(body, posted_body));
  assert(cell(body, 1, 1) == "-5.2" && cell(body, 1, 2) == "N/A" && cell(body, 1, 3) == "N/A");
  assert(cell(body, 5, 4) == "N/A");

  // Other notifications retain the original text-only request shape and timestamp.
  assert(strcmp(slack_send_message(time, "[INFO] HM OFF"), "123.456") == 0);
  assert(!deserializeJson(body, posted_body));
  assert(body["blocks"].isNull());
  assert(strcmp(body["text"], "12:34:56\n[INFO] HM OFF") == 0);
  post_status = 500;
  assert(slack_send_environment_report(time, sensors, heading, details)[0] == '\0');
  posted_body = ""; wifi_connected = false;
  assert(slack_send_message(time, "test")[0] == '\0' && posted_body.empty());
  std::cout << "Slack report tests passed: native table payload, units, missing values, fallback and notification compatibility\n";
}
