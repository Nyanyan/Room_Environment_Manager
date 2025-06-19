#include <WiFi.h>
#include "esp_wpa2.h"
// #include "esp_eap_client.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "slack.h"


// WiFi password & Slack token
#include "token.h"



// json
StaticJsonDocument<1024> doc;

// void init_wifi(){
//   WiFi.begin(WIFI_SSID, WIFI_PASS);
//   while (WiFi.status() != WL_CONNECTED) {
//     Serial.print(".");
//     delay(100);
//   }
//   Serial.println("WiFi connected");
// }

bool init_wifi_try() {
  WiFi.disconnect(true);  // 接続状態をリセット
  WiFi.mode(WIFI_STA);    // ステーションモードに設定

  // WPA2-Enterpriseの設定
  esp_wifi_sta_wpa2_ent_set_identity((uint8_t *)EXH_WIFI_ID, strlen(EXH_WIFI_ID));
  esp_wifi_sta_wpa2_ent_set_username((uint8_t *)EXH_WIFI_ID, strlen(EXH_WIFI_ID));
  esp_wifi_sta_wpa2_ent_set_password((uint8_t *)EXH_WIFI_PASS, strlen(EXH_WIFI_PASS));
  esp_wifi_sta_wpa2_ent_enable();  // WPA2-Enterpriseを有効にする

  WiFi.begin(EXH_WIFI_SSID);

  Serial.println("Connecting to WPA2 Enterprise WiFi...");
  for (int i = 0; i < 20 && WiFi.status() != WL_CONNECTED; ++i) {
    delay(500);
    Serial.print(".");
  }
  
  return WiFi.status() == WL_CONNECTED;
}

void init_wifi(){
  
  while (!init_wifi_try()) {
    Serial.println("trying...");
  }
  
  Serial.println("");
  Serial.println("Connected!");
  Serial.println(WiFi.localIP());
  Serial.println("WiFi connected");
}



String slack_get_message() {
  HTTPClient http;
  String res;

  if (http.begin(SLACK_URL_RECEIVE)) {
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
  } else {
    Serial.printf("[HTTPS] Unable to connect\n");
  }
  return res;
}



char* slack_send_message(Time_info &time_info, String str){
  static char ts[20];
  char buf[2048];

  // Slack Messaging API
  HTTPClient http;
  if (!http.begin(SLACK_URL_SEND)) {
    Serial.println(String("[ERROR] cannot begin ") + SLACK_URL_SEND);
    return "";
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
  String body, received_string;
  int status_code;

  // files.getUploadURLExternal
  if (!http.begin(SLACK_URL_GET_UPLOAD_URL)) {
    Serial.println(String("[ERROR] cannot begin ") + SLACK_URL_GET_UPLOAD_URL);
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