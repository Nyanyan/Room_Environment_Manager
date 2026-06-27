#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <stdlib.h>
#include <string.h>
#include "display.h"
#include "time_manager.h"
#include "slack.h"

// WiFi password & Slack token
#include "token.h"

#ifndef SLACK_CHANNEL_ID
#define SLACK_CHANNEL_ID SLACK_CHANNEL
#endif

#define SLACK_CONNECT_N_TRY 3
#define SLACK_HTTP_TIMEOUT_MS 8000
#define WIFI_CONNECT_MAX_TRY 3

static unsigned long g_last_wifi_connect_ms = 0; // track last successful WiFi association

void init_wifi();

static bool is_http_success(int status_code) {
  return status_code == HTTP_CODE_OK || status_code == HTTP_CODE_MOVED_PERMANENTLY;
}

static char hex_digit(uint8_t value) {
  return value < 10 ? static_cast<char>('0' + value) : static_cast<char>('A' + value - 10);
}

static String url_encode(const String &value) {
  String encoded;
  encoded.reserve(value.length() * 3);
  for (size_t i = 0; i < value.length(); ++i) {
    uint8_t c = static_cast<uint8_t>(value[i]);
    if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') ||
        c == '-' || c == '_' || c == '.' || c == '~') {
      encoded += static_cast<char>(c);
    } else if (c == ' ') {
      encoded += '+';
    } else {
      encoded += '%';
      encoded += hex_digit((c >> 4) & 0x0F);
      encoded += hex_digit(c & 0x0F);
    }
  }
  return encoded;
}

static void add_slack_auth_header(HTTPClient &http, const char *token) {
  http.addHeader("Authorization", String("Bearer ") + token);
}

static bool begin_http_with_retries(HTTPClient &http, const char *url) {
  http.setTimeout(SLACK_HTTP_TIMEOUT_MS);
  http.setReuse(false);
  for (int i = 0; i < SLACK_CONNECT_N_TRY; ++i) {
    if (http.begin(url)) {
      return true;
    }
    Serial.println(String("[ERROR] cannot begin ") + url + String(", reconnecting WiFi..."));
    init_wifi();
  }
  return false;
}

static bool parse_slack_ok(const String &json, JsonDocument &doc, const char *context) {
  doc.clear();
  DeserializationError err = deserializeJson(doc, json);
  if (err) {
    Serial.println(String("[ERROR] ") + context + String(" JSON parse failed: ") + err.c_str());
    return false;
  }
  if (doc["ok"] != true) {
    const char *error = doc["error"] | "unknown_error";
    Serial.println(String("[ERROR] ") + context + String(" Slack error: ") + error);
    return false;
  }
  return true;
}

void init_wifi(){
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);

  while (true) {
    for (int attempt = 0; attempt < WIFI_CONNECT_MAX_TRY; ++attempt) {
      WiFi.disconnect(false, false);
      delay(100);
      WiFi.begin(WIFI_SSID, WIFI_PASS);
      unsigned long start = millis();
      while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
        Serial.print(".");
        delay(100);
      }
      if (WiFi.status() == WL_CONNECTED) {
        Serial.println("WiFi connected");
        g_last_wifi_connect_ms = millis();
        return;
      }
    }

    // Show failure on LCD after repeated attempts
    display_clear();
    display_print(0, 0, "Cannot Connect");
    display_print(0, 1, "to WiFi");
    display_print(0, 2, WIFI_SSID);
    display_print(0, 3, WIFI_PASS);
    Serial.println("[ERROR] Cannot Connect to WiFi");
  }
}

static bool ensure_wifi_connected_and_settled() {
  if (WiFi.status() != WL_CONNECTED) {
    init_wifi();
  }
  if (g_last_wifi_connect_ms == 0) {
    return WiFi.status() == WL_CONNECTED;
  }
  const unsigned long grace_ms = 1500; // allow link/DHCP to stabilize before HTTPS
  unsigned long elapsed = millis() - g_last_wifi_connect_ms;
  if (elapsed < grace_ms) {
    delay(grace_ms - elapsed);
  }
  return WiFi.status() == WL_CONNECTED;
}

String slack_get_message() {
  String res;
  if (!ensure_wifi_connected_and_settled()) {
    Serial.println("[ERROR] WiFi not connected; skip slack_get_message");
    return res;
  }

  HTTPClient http;
  if (!begin_http_with_retries(http, SLACK_URL_RECEIVE)) {
    Serial.println("[ERROR] Slack conversations.history begin failed");
    return res;
  }

  add_slack_auth_header(http, SLACK_TOKEN);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  String body = String("channel=") + url_encode(SLACK_CHANNEL_ID) + "&limit=1";
  int status_code = http.POST(body);
  Serial.printf("[HTTPS] conversations.history code: %d\n", status_code);
  if (is_http_success(status_code)) {
    String raw_json = http.getString();
    JsonDocument doc;
    if (parse_slack_ok(raw_json, doc, "conversations.history")) {
      const char* text = doc["messages"][0]["text"] | "";
      res = text;
    }
  } else {
    Serial.printf("[HTTPS] conversations.history failed, error: %s\n", http.errorToString(status_code).c_str());
  }
  http.end();
  return res;
}

char* slack_send_message(Time_info &time_info, String str){
  static char ts[32];
  strcpy(ts, "");

  if (!ensure_wifi_connected_and_settled()) {
    Serial.println("[ERROR] WiFi not connected; skip slack_send_message");
    return ts;
  }

  HTTPClient http;
  if (!begin_http_with_retries(http, SLACK_URL_SEND)) {
    Serial.println("[ERROR] slack_send_message giving up after retries");
    return ts;
  }

  String str_with_timestamp = String(time_info.time_str) + "\n" + str;
  Serial.println(str_with_timestamp);

  JsonDocument body_doc;
  body_doc["channel"] = SLACK_CHANNEL_ID;
  body_doc["text"] = str_with_timestamp;
  String body;
  serializeJson(body_doc, body);

  add_slack_auth_header(http, SLACK_TOKEN);
  http.addHeader("Content-Type", "application/json; charset=utf-8");
  int status_code = http.POST(body);
  Serial.printf("[HTTPS] chat.postMessage code: %d\n", status_code);
  if (is_http_success(status_code)) {
    String json = http.getString();
    JsonDocument doc;
    if (parse_slack_ok(json, doc, "chat.postMessage")) {
      const char *response_ts = doc["ts"] | "";
      strncpy(ts, response_ts, sizeof(ts) - 1);
      ts[sizeof(ts) - 1] = '\0';
      Serial.println(ts);
    }
  } else {
    Serial.printf("[HTTPS] chat.postMessage failed, error: %s\n", http.errorToString(status_code).c_str());
  }
  http.end();
  return ts;
}

String slack_upload_img(uint8_t *img_data, uint32_t img_file_size, String img_file_name){
  String body, received_string;
  int status_code;

  if (!ensure_wifi_connected_and_settled()) {
    Serial.println("[ERROR] WiFi not connected; skip slack_upload_img");
    return String("");
  }

  String upload_url;
  String file_id;
  {
    HTTPClient http;
    if (!begin_http_with_retries(http, SLACK_URL_GET_UPLOAD_URL)) {
      Serial.println("[ERROR] slack_upload_img getUploadURLExternal begin failed");
      return String("");
    }
    add_slack_auth_header(http, SLACK_TOKEN);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    body = String("length=") + String(img_file_size) + "&filename=" + url_encode(img_file_name);
    status_code = http.POST(body);
    if (!is_http_success(status_code)) {
      Serial.println("[ERROR] getUploadURLExternal status code error: " + String(status_code));
      http.end();
      return String("");
    }
    received_string = http.getString();
    JsonDocument doc;
    if (!parse_slack_ok(received_string, doc, "getUploadURLExternal")) {
      http.end();
      return String("");
    }
    upload_url = doc["upload_url"] | "";
    file_id = doc["file_id"] | "";
    Serial.println(String("upload_url: ") + upload_url);
    Serial.println(String("file_id: ") + file_id);
    http.end();
  }

  if (upload_url.length() == 0 || file_id.length() == 0) {
    Serial.println("[ERROR] upload url or file id is empty");
    return String("");
  }

  {
    HTTPClient http;
    http.setTimeout(SLACK_HTTP_TIMEOUT_MS);
    http.setReuse(false);
    if (!http.begin(upload_url)) {
      Serial.println(String("[ERROR] cannot begin ") + upload_url);
      return String("");
    }
    http.addHeader("Content-Type", String("multipart/form-data; boundary=") + HTTP_BOUNDARY);
    String multipart_header = String("--") + HTTP_BOUNDARY + "\r\n";
    multipart_header += String("Content-Disposition: form-data; name=\"uploadFile\"; filename=\"") + img_file_name + "\"\r\n";
    multipart_header += "\r\n";
    String multipart_footer = "\r\n";
    multipart_footer += String("--") + HTTP_BOUNDARY + "--\r\n";
    multipart_footer += "\r\n";

    uint32_t multipart_header_size = multipart_header.length();
    uint32_t multipart_footer_size = multipart_footer.length();
    uint32_t multipart_all_size = multipart_header_size + img_file_size + multipart_footer_size;
    Serial.println("send " + String(multipart_all_size) + " bytes");
    uint8_t* multipart_data = (uint8_t*)malloc(sizeof(uint8_t) * multipart_all_size);
    if (multipart_data == nullptr) {
      Serial.println("[ERROR] multipart malloc failed");
      http.end();
      return String("");
    }
    memcpy(multipart_data, multipart_header.c_str(), multipart_header_size);
    memcpy(multipart_data + multipart_header_size, img_data, img_file_size);
    memcpy(multipart_data + multipart_header_size + img_file_size, multipart_footer.c_str(), multipart_footer_size);

    status_code = http.POST(multipart_data, multipart_all_size);
    free(multipart_data);
    if (!is_http_success(status_code)) {
      Serial.println("[ERROR] upload image status code error: " + String(status_code));
      http.end();
      return String("");
    }
    received_string = http.getString();
    Serial.println(String("POST result: ") + received_string);
    http.end();
  }

  {
    HTTPClient http;
    if (!begin_http_with_retries(http, SLACK_URL_COMPLETE_UPLOAD)) {
      Serial.println("[ERROR] slack_upload_img completeUploadExternal begin failed");
      return String("");
    }
    add_slack_auth_header(http, SLACK_TOKEN);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    String file_data = "[{\"id\":\"" + file_id + "\", \"title\":\"" + img_file_name + "\"}]";
    body = String("files=") + url_encode(file_data);
    status_code = http.POST(body);
    if (!is_http_success(status_code)) {
      Serial.println("[ERROR] completeUploadExternal status code error: " + String(status_code));
      http.end();
      return String("");
    }
    received_string = http.getString();
    Serial.println(String("completeUploadExternal result: ") + received_string);
    JsonDocument doc;
    if (!parse_slack_ok(received_string, doc, "completeUploadExternal")) {
      http.end();
      return String("");
    }
    const char* permalink = doc["files"][0]["permalink"] | "";
    String permalink_str = permalink;
    Serial.println(String("permalink: ") + permalink_str);
    http.end();
    return permalink_str;
  }
}
