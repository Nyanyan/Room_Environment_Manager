#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "time_manager.h"
#include "slack.h"
#include "display.h"


// WiFi password & Slack token
#include "token.h"

#define SLACK_CONNECT_N_TRY 3
#define SLACK_HTTP_TIMEOUT_MS 5000
#define WIFI_CONNECT_MAX_TRY 3



// json
StaticJsonDocument<1024> doc;
static unsigned long g_last_wifi_connect_ms = 0; // track last successful WiFi association

void init_wifi(){
  while (true) {
    for (int attempt = 0; attempt < WIFI_CONNECT_MAX_TRY; ++attempt) {
      WiFi.begin(WIFI_SSID, WIFI_PASS);
      unsigned long long strt = millis();
      while (WiFi.status() != WL_CONNECTED && millis() - strt < 10000) {
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
  HTTPClient http;
  http.setTimeout(SLACK_HTTP_TIMEOUT_MS);
  String res;

  if (!ensure_wifi_connected_and_settled()) {
    Serial.println("[ERROR] WiFi not connected; skip slack_get_message");
    return res;
  }

  bool begin_ok = false;
  for (int i = 0; i < SLACK_CONNECT_N_TRY; ++i) {
    if (http.begin(SLACK_URL_RECEIVE)) {
      begin_ok = true;
      break;
    }
    Serial.printf("[HTTPS] Unable to connect, initializing wifi...\n");
    init_wifi();
    ensure_wifi_connected_and_settled();
  }
  if (!begin_ok) {
    Serial.println("[ERROR] Slack conversations.history begin failed");
    return res;
  }

  // if (http.begin(SLACK_URL_RECEIVE)) {
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  String body = String("token=") + SLACK_TOKEN + "&channel=" + SLACK_CHANNEL_ID + "&limit=1";
  int status_code = http.POST(body);
  Serial.printf("[HTTPS] POST... code: %d\n", status_code);
  if (status_code == HTTP_CODE_OK || status_code == HTTP_CODE_MOVED_PERMANENTLY) {
    String raw_json = http.getString();
    deserializeJson(doc, raw_json);
    const char* text = doc["messages"][0]["text"];
    res = text;
  } else{
    Serial.printf("[HTTPS] POST... failed, error: %s\n", http.errorToString(status_code).c_str());
  }
  http.end();
  // } else {
  //   Serial.printf("[HTTPS] Unable to connect\n");
  // }
  return res;
}



char* slack_send_message(Time_info &time_info, String str){
  static char ts[20];
  char buf[2048];

  // Slack Messaging API
  HTTPClient http;
  http.setTimeout(SLACK_HTTP_TIMEOUT_MS);
  bool begin_ok = false;

  if (!ensure_wifi_connected_and_settled()) {
    Serial.println("[ERROR] WiFi not connected; skip slack_send_message");
    return ts;
  }

  for (int i = 0; i < SLACK_CONNECT_N_TRY; ++i) {
    if (http.begin(SLACK_URL_SEND)) {
      begin_ok = true;
      break;
    }
    Serial.println(String("[ERROR] cannot begin ") + SLACK_URL_SEND + String(", initializing wifi..."));
    init_wifi();
    ensure_wifi_connected_and_settled();
    // return "";
  }
  if (!begin_ok) {
    Serial.println("[ERROR] slack_send_message giving up after retries");
    return ts;
  }
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  String str_with_timestamp = String(time_info.time_str) + "\n" + str;
  const char *str_c = str_with_timestamp.c_str();
  Serial.println(str_c);

  sprintf(buf, ""
               "token=%s"
               "&channel=%s"
               "&text=%s",
          SLACK_TOKEN,
          SLACK_CHANNEL,
          str_c);

  strcpy(ts, "");

  int status_code = http.POST((uint8_t*)buf, strlen(buf));
  Serial.print(status_code);
  if (status_code == HTTP_CODE_OK || status_code == HTTP_CODE_MOVED_PERMANENTLY) {
    String json = http.getString();
    Serial.println(json);
    deserializeJson(doc, json);
    if (doc.containsKey("ts")) {
      strcpy(ts, doc["ts"]);
      Serial.println(ts);
    }
  } else {
    Serial.printf("ERR %d", status_code);
  }
  http.end();
  return ts;
}



String slack_upload_img(uint8_t *img_data, uint32_t img_file_size, String img_file_name){
  HTTPClient http;
  http.setTimeout(SLACK_HTTP_TIMEOUT_MS);
  String body, received_string;
  int status_code;

  if (!ensure_wifi_connected_and_settled()) {
    Serial.println("[ERROR] WiFi not connected; skip slack_upload_img");
    return String("");
  }

  // files.getUploadURLExternal
  bool begin_ok = false;
  for (int i = 0; i < SLACK_CONNECT_N_TRY; ++i) {
    if (http.begin(SLACK_URL_GET_UPLOAD_URL)) {
      begin_ok = true;
      break;
    }
    Serial.println(String("[ERROR] cannot begin ") + SLACK_URL_GET_UPLOAD_URL + String(", initializing wifi..."));
    init_wifi();
    ensure_wifi_connected_and_settled();
    // return String("");
  }
  if (!begin_ok) {
    Serial.println("[ERROR] slack_upload_img giving up after retries");
    return String("");
  }
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  body = String("token=") + SLACK_TOKEN + "&length=" + String(img_file_size) + "&filename=" + img_file_name;
  status_code = http.POST(body);
  if (status_code != HTTP_CODE_OK && status_code != HTTP_CODE_MOVED_PERMANENTLY) {
    Serial.println("[ERROR] status code error: " + String(status_code));
    return String("");
  }
  received_string = http.getString();
  deserializeJson(doc, received_string);
  if (doc["ok"] != true) {
    Serial.print("[ERROR] getUploadURLExternal slack response is not ok");
    return String("");
  }
  const char* upload_url = doc["upload_url"];
  const char* file_id = doc["file_id"];
  Serial.println(String("upload_url: ") + upload_url);
  Serial.println(String("file_id: ") + file_id);
  http.end();

  // upload image to upload_url
  if (!http.begin(upload_url)) {
    Serial.println(String("[ERROR] cannot begin ") + upload_url);
    return String("");
  }
  http.addHeader("Content-Type", String("multipart/form-data; boundary=") + HTTP_BOUNDARY);
  // create multipart header
  String multipart_header = "";
  multipart_header += String("--") + HTTP_BOUNDARY + "\r\n";
  multipart_header += String("Content-Disposition: form-data; name=\"uploadFile\"; filename=\"") + img_file_name + "\"\r\n";
  multipart_header += "\r\n";
  // create multipart footer
  String multipart_footer = "";
  multipart_footer += "\r\n";
  multipart_footer += String("--") + HTTP_BOUNDARY + "--\r\n";
  multipart_footer += "\r\n";
  // concatenate data
  uint32_t multipart_header_size = multipart_header.length();
  uint32_t multipart_footer_size = multipart_footer.length();
  uint32_t multipart_all_size = multipart_header_size + img_file_size + multipart_footer_size;
  Serial.println("send " + String(multipart_all_size) + " bytes");
  uint8_t* multipart_data = (uint8_t*)malloc(sizeof(uint8_t) * multipart_all_size);
  for (int i = 0; i < multipart_header_size; ++i) {
    multipart_data[i] = multipart_header[i];
  }
  for (int i = 0; i < img_file_size; ++i) {
    multipart_data[multipart_header_size + i] = img_data[i];
  }
  for (int i = 0; i < multipart_footer_size; ++i) {
    multipart_data[multipart_header_size + img_file_size + i] = multipart_footer[i];
  }
  // POST data
  status_code = http.POST(multipart_data, multipart_all_size);
  free(multipart_data);
  if (status_code != HTTP_CODE_OK && status_code != HTTP_CODE_MOVED_PERMANENTLY) {
    Serial.println("[ERROR] status code error: " + String(status_code));
    return String("");
  }
  received_string = http.getString();
  Serial.println(String("POST result: ") + received_string);
  http.end();

  // files.completeUploadExternal
  if (!http.begin(SLACK_URL_COMPLETE_UPLOAD)) {
    Serial.println(String("[ERROR] cannot begin ") + SLACK_URL_COMPLETE_UPLOAD);
    return String("");
  }
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  String file_data = "[{\"id\":\"" + String(file_id) + "\", \"title\":\"" + img_file_name + "\"}]";
  //body = String("token=") + SLACK_TOKEN + "&channel_id=" + String(SLACK_CHANNEL_ID) + "&files=" + file_data;
  body = String("token=") + SLACK_TOKEN + "&files=" + file_data;
  status_code = http.POST(body);
  if (status_code != HTTP_CODE_OK && status_code != HTTP_CODE_MOVED_PERMANENTLY) {
    Serial.println("[ERROR] status code error: " + String(status_code));
    return String("");
  }
  received_string = http.getString();
  Serial.println(String("completeUploadExternal result: ") + received_string);
  deserializeJson(doc, received_string);
  if (doc["ok"] != true) {
    Serial.print("[ERROR] completeUploadExternal slack response is not ok");
    return String("");
  }
  const char* permalink = doc["files"][0]["permalink"];
  Serial.println(permalink);
  Serial.println(String("permalink: ") + permalink);
  http.end();
  return permalink;
}