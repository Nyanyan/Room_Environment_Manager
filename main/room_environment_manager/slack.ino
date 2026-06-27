#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <esp_system.h>
#include <mbedtls/base64.h>
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
#ifndef SLACK_APP_TOKEN
#define SLACK_APP_TOKEN ""
#endif

#define SLACK_CONNECT_N_TRY 3
#define SLACK_HTTP_TIMEOUT_MS 8000
#define WIFI_CONNECT_MAX_TRY 3

static unsigned long g_last_wifi_connect_ms = 0; // track last successful WiFi association

void init_wifi();
static bool ensure_wifi_connected_and_settled();

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

#define SLACK_COMMAND_QUEUE_SIZE 4
static String g_slack_command_queue[SLACK_COMMAND_QUEUE_SIZE];
static uint8_t g_slack_command_head = 0;
static uint8_t g_slack_command_count = 0;

static void enqueue_slack_command(const String &message) {
  if (message.length() == 0) {
    return;
  }
  if (g_slack_command_count >= SLACK_COMMAND_QUEUE_SIZE) {
    g_slack_command_head = (g_slack_command_head + 1) % SLACK_COMMAND_QUEUE_SIZE;
    g_slack_command_count--;
  }
  uint8_t idx = (g_slack_command_head + g_slack_command_count) % SLACK_COMMAND_QUEUE_SIZE;
  g_slack_command_queue[idx] = message;
  g_slack_command_count++;
}

static String dequeue_slack_command() {
  if (g_slack_command_count == 0) {
    return String("");
  }
  String message = g_slack_command_queue[g_slack_command_head];
  g_slack_command_queue[g_slack_command_head] = "";
  g_slack_command_head = (g_slack_command_head + 1) % SLACK_COMMAND_QUEUE_SIZE;
  g_slack_command_count--;
  return message;
}

static void trim_spaces(String &text) {
  text.trim();
}

static String normalize_slack_command_text(const char *raw_text) {
  String text = raw_text == nullptr ? String("") : String(raw_text);
  trim_spaces(text);
  return text;
}

static bool slack_app_token_configured() {
  return strlen(SLACK_APP_TOKEN) > 0;
}

static WiFiClientSecure g_slack_socket;
static bool g_slack_socket_started = false;
static unsigned long g_next_socket_connect_ms = 0;
static unsigned long g_last_socket_rx_ms = 0;
static unsigned long g_last_socket_ping_ms = 0;
static String g_socket_host;
static String g_socket_path;
static uint16_t g_socket_port = 443;
static String g_last_slack_event_ts;

static bool parse_wss_url(const String &url, String &host, uint16_t &port, String &path) {
  const String prefix = "wss://";
  if (!url.startsWith(prefix)) {
    return false;
  }
  int host_start = prefix.length();
  int path_start = url.indexOf('/', host_start);
  if (path_start < 0) {
    return false;
  }
  host = url.substring(host_start, path_start);
  path = url.substring(path_start);
  port = 443;
  int colon = host.indexOf(':');
  if (colon >= 0) {
    port = static_cast<uint16_t>(host.substring(colon + 1).toInt());
    host = host.substring(0, colon);
  }
  return host.length() > 0 && path.length() > 0;
}

static String make_websocket_key() {
  uint8_t nonce[16];
  for (size_t i = 0; i < sizeof(nonce); ++i) {
    nonce[i] = static_cast<uint8_t>(esp_random() & 0xFF);
  }

  unsigned char encoded[32];
  size_t encoded_len = 0;
  int rc = mbedtls_base64_encode(encoded, sizeof(encoded), &encoded_len, nonce, sizeof(nonce));
  if (rc != 0 || encoded_len == 0 || encoded_len >= sizeof(encoded)) {
    return String("dGhlIHNhbXBsZSBub25jZQ==");
  }
  encoded[encoded_len] = '\0';
  return String(reinterpret_cast<const char *>(encoded));
}

static bool read_socket_bytes(uint8_t *buffer, size_t length, unsigned long timeout_ms) {
  size_t offset = 0;
  unsigned long start = millis();
  while (offset < length) {
    if (!g_slack_socket.connected()) {
      return false;
    }
    int available = g_slack_socket.available();
    if (available > 0) {
      int n = g_slack_socket.read(buffer + offset, length - offset);
      if (n > 0) {
        offset += static_cast<size_t>(n);
        start = millis();
        continue;
      }
    }
    if (millis() - start >= timeout_ms) {
      return false;
    }
    delay(1);
  }
  return true;
}

static bool read_websocket_handshake() {
  String response;
  unsigned long start = millis();
  while (millis() - start < 5000) {
    while (g_slack_socket.available() > 0) {
      char c = static_cast<char>(g_slack_socket.read());
      response += c;
      if (response.endsWith("\r\n\r\n")) {
        return response.startsWith("HTTP/1.1 101") || response.startsWith("HTTP/1.0 101");
      }
      if (response.length() > 2048) {
        return false;
      }
    }
    if (!g_slack_socket.connected()) {
      return false;
    }
    delay(1);
  }
  return false;
}

static bool websocket_send_frame(uint8_t opcode, const uint8_t *payload, size_t length) {
  if (!g_slack_socket.connected() || length > 65535) {
    return false;
  }

  uint8_t header[8];
  size_t header_len = 0;
  header[header_len++] = 0x80 | (opcode & 0x0F);
  if (length <= 125) {
    header[header_len++] = 0x80 | static_cast<uint8_t>(length);
  } else {
    header[header_len++] = 0x80 | 126;
    header[header_len++] = static_cast<uint8_t>((length >> 8) & 0xFF);
    header[header_len++] = static_cast<uint8_t>(length & 0xFF);
  }

  uint8_t mask[4];
  uint32_t rnd = esp_random();
  mask[0] = static_cast<uint8_t>((rnd >> 24) & 0xFF);
  mask[1] = static_cast<uint8_t>((rnd >> 16) & 0xFF);
  mask[2] = static_cast<uint8_t>((rnd >> 8) & 0xFF);
  mask[3] = static_cast<uint8_t>(rnd & 0xFF);

  if (g_slack_socket.write(header, header_len) != header_len ||
      g_slack_socket.write(mask, sizeof(mask)) != sizeof(mask)) {
    return false;
  }

  uint8_t chunk[64];
  size_t offset = 0;
  while (offset < length) {
    size_t n = length - offset;
    if (n > sizeof(chunk)) {
      n = sizeof(chunk);
    }
    for (size_t i = 0; i < n; ++i) {
      chunk[i] = payload[offset + i] ^ mask[(offset + i) % 4];
    }
    if (g_slack_socket.write(chunk, n) != n) {
      return false;
    }
    offset += n;
  }
  return true;
}

static bool websocket_send_text(const String &text) {
  return websocket_send_frame(0x1, reinterpret_cast<const uint8_t *>(text.c_str()), text.length());
}

static void slack_socket_ack(const char *envelope_id) {
  if (envelope_id == nullptr || envelope_id[0] == '\0') {
    return;
  }
  JsonDocument ack_doc;
  ack_doc["envelope_id"] = envelope_id;
  String ack;
  serializeJson(ack_doc, ack);
  websocket_send_text(ack);
}

static void close_slack_socket(unsigned long reconnect_delay_ms) {
  if (g_slack_socket.connected()) {
    websocket_send_frame(0x8, nullptr, 0);
  }
  g_slack_socket.stop();
  g_slack_socket_started = false;
  g_next_socket_connect_ms = millis() + reconnect_delay_ms;
}

static void handle_slack_socket_text(const String &message) {
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, message);
  if (err) {
    Serial.println(String("[ERROR] Slack socket JSON parse failed: ") + err.c_str());
    return;
  }

  const char *wrapper_type = doc["type"] | "";
  if (strcmp(wrapper_type, "hello") == 0) {
    Serial.println("[INFO] Slack Socket Mode connected");
    return;
  }
  if (strcmp(wrapper_type, "disconnect") == 0) {
    Serial.println("[INFO] Slack requested Socket Mode reconnect");
    close_slack_socket(1000);
    return;
  }
  if (strcmp(wrapper_type, "events_api") != 0) {
    return;
  }

  const char *envelope_id = doc["envelope_id"] | "";
  slack_socket_ack(envelope_id);

  JsonObject event = doc["payload"]["event"];
  const char *event_type = event["type"] | "";
  const char *subtype = event["subtype"] | "";
  const char *channel = event["channel"] | "";
  const char *text = event["text"] | "";
  const char *event_ts = event["event_ts"] | "";

  if (strcmp(event_type, "message") != 0) {
    return;
  }
  if (strcmp(channel, SLACK_CHANNEL_ID) != 0) {
    return;
  }
  if (subtype[0] != '\0') {
    return;
  }

  String command_text = normalize_slack_command_text(text);
  if (command_text.length() == 0) {
    return;
  }
  if (event_ts[0] != '\0' && g_last_slack_event_ts == event_ts) {
    return;
  }

  g_last_slack_event_ts = event_ts;
  enqueue_slack_command(command_text);
  Serial.println(String("[INFO] Slack command queued: ") + command_text);
}

static bool read_slack_socket_frame() {
  if (!g_slack_socket.connected()) {
    return false;
  }
  if (g_slack_socket.available() < 2) {
    return false;
  }

  uint8_t header[2];
  if (!read_socket_bytes(header, sizeof(header), 1000)) {
    close_slack_socket(5000);
    return false;
  }

  uint8_t opcode = header[0] & 0x0F;
  bool masked = (header[1] & 0x80) != 0;
  uint64_t length = header[1] & 0x7F;
  if (length == 126) {
    uint8_t ext[2];
    if (!read_socket_bytes(ext, sizeof(ext), 1000)) {
      close_slack_socket(5000);
      return false;
    }
    length = (static_cast<uint16_t>(ext[0]) << 8) | ext[1];
  } else if (length == 127) {
    uint8_t ext[8];
    if (!read_socket_bytes(ext, sizeof(ext), 1000)) {
      close_slack_socket(5000);
      return false;
    }
    length = 0;
    for (size_t i = 0; i < sizeof(ext); ++i) {
      length = (length << 8) | ext[i];
    }
  }

  if (length > 8192) {
    Serial.println("[ERROR] Slack socket frame too large");
    close_slack_socket(5000);
    return false;
  }

  uint8_t mask[4] = {0, 0, 0, 0};
  if (masked && !read_socket_bytes(mask, sizeof(mask), 1000)) {
    close_slack_socket(5000);
    return false;
  }

  String payload;
  payload.reserve(static_cast<unsigned int>(length));
  for (uint64_t i = 0; i < length; ++i) {
    uint8_t b = 0;
    if (!read_socket_bytes(&b, 1, 1000)) {
      close_slack_socket(5000);
      return false;
    }
    if (masked) {
      b ^= mask[i % 4];
    }
    payload += static_cast<char>(b);
  }

  g_last_socket_rx_ms = millis();
  if (opcode == 0x1) {
    handle_slack_socket_text(payload);
  } else if (opcode == 0x8) {
    Serial.println("[WARN] Slack Socket Mode closed");
    close_slack_socket(5000);
  } else if (opcode == 0x9) {
    websocket_send_frame(0xA, reinterpret_cast<const uint8_t *>(payload.c_str()), payload.length());
  }
  return true;
}

static bool slack_socket_open() {
  if (!slack_app_token_configured()) {
    return false;
  }
  if (!ensure_wifi_connected_and_settled()) {
    Serial.println("[ERROR] WiFi not connected; skip Slack Socket Mode open");
    return false;
  }

  HTTPClient http;
  if (!begin_http_with_retries(http, SLACK_URL_SOCKET_OPEN)) {
    Serial.println("[ERROR] apps.connections.open begin failed");
    return false;
  }
  add_slack_auth_header(http, SLACK_APP_TOKEN);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  int status_code = http.POST("");
  if (!is_http_success(status_code)) {
    Serial.println("[ERROR] apps.connections.open status code error: " + String(status_code));
    http.end();
    return false;
  }

  String json = http.getString();
  JsonDocument doc;
  if (!parse_slack_ok(json, doc, "apps.connections.open")) {
    http.end();
    return false;
  }
  String socket_url = doc["url"] | "";
  http.end();

  if (!parse_wss_url(socket_url, g_socket_host, g_socket_port, g_socket_path)) {
    Serial.println("[ERROR] invalid Slack Socket Mode URL");
    return false;
  }

  g_slack_socket.stop();
  g_slack_socket.setInsecure();
  g_slack_socket.setTimeout(1000);
  if (!g_slack_socket.connect(g_socket_host.c_str(), g_socket_port)) {
    Serial.println("[ERROR] Slack Socket Mode TCP connect failed");
    return false;
  }

  String websocket_key = make_websocket_key();
  String request = String("GET ") + g_socket_path + String(" HTTP/1.1\r\n") +
    String("Host: ") + g_socket_host + String("\r\n") +
    String("Upgrade: websocket\r\n") +
    String("Connection: Upgrade\r\n") +
    String("Sec-WebSocket-Key: ") + websocket_key + String("\r\n") +
    String("Sec-WebSocket-Version: 13\r\n\r\n");
  g_slack_socket.print(request);

  if (!read_websocket_handshake()) {
    Serial.println("[ERROR] Slack Socket Mode handshake failed");
    g_slack_socket.stop();
    return false;
  }

  g_slack_socket_started = true;
  g_last_socket_rx_ms = millis();
  g_last_socket_ping_ms = millis();
  Serial.println("[INFO] Slack Socket Mode websocket opened");
  return true;
}

static void slack_socket_service() {
  static bool warned_missing_token = false;
  if (!slack_app_token_configured()) {
    if (!warned_missing_token) {
      Serial.println("[ERROR] SLACK_APP_TOKEN is not set; Slack Socket Mode disabled");
      warned_missing_token = true;
    }
    return;
  }

  unsigned long now = millis();
  if (!g_slack_socket_started && now >= g_next_socket_connect_ms) {
    if (!slack_socket_open()) {
      g_next_socket_connect_ms = now + 30000;
      return;
    }
  }
  if (!g_slack_socket_started) {
    return;
  }

  if (!g_slack_socket.connected()) {
    Serial.println("[WARN] Slack Socket Mode disconnected");
    close_slack_socket(5000);
    return;
  }

  for (uint8_t i = 0; i < 4 && g_slack_socket.available() > 0; ++i) {
    read_slack_socket_frame();
  }

  now = millis();
  if (now - g_last_socket_ping_ms > 30000) {
    websocket_send_frame(0x9, nullptr, 0);
    g_last_socket_ping_ms = now;
  }
  if (now - g_last_socket_rx_ms > 90000) {
    Serial.println("[WARN] Slack Socket Mode idle timeout");
    close_slack_socket(5000);
  }
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

void slack_maintain() {
  slack_socket_service();
}

void slack_delay(unsigned long delay_ms) {
  const unsigned long step_ms = 50;
  unsigned long start = millis();
  while (millis() - start < delay_ms) {
    slack_maintain();
    unsigned long elapsed = millis() - start;
    if (elapsed >= delay_ms) {
      break;
    }
    unsigned long remaining = delay_ms - elapsed;
    delay(remaining < step_ms ? remaining : step_ms);
  }
}

String slack_get_message() {
  slack_maintain();
  return dequeue_slack_command();
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