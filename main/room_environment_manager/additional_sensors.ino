#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <cstring>
#include <float.h>
#include "additional_sensors.h"

// Forward declaration from slack.ino
void init_wifi();

// ESP-NOW channel shared with the additional sensor nodes.
static const uint8_t ESPNOW_CHANNEL = 1;
static const unsigned long ADDITIONAL_SENSOR_MAX_AGE_MS = 5UL * 60UL * 1000UL; // 5 minutes
static const unsigned long ADDITIONAL_SENSOR_REQUEST_INTERVAL_MS = 60UL * 1000UL; // limit WiFi tear-down to at most once per minute

struct AdditionalSensorPacket {
  char header[N_SLAVE_HEADER];
  float temperature_c;
  float humidity_pct;
  float pressure_hpa;
};

static SensorReading g_additional_sensor_data[N_ADDITIONAL_SENSORS];
static bool g_additional_sensor_received[N_ADDITIONAL_SENSORS];
static SensorReading g_additional_sensor_last_data[N_ADDITIONAL_SENSORS];
static unsigned long g_additional_sensor_last_ms[N_ADDITIONAL_SENSORS];
static unsigned long g_last_additional_request_ms = 0;

static void reset_wifi_for_espnow() {
  WiFi.persistent(false);
  WiFi.disconnect(true, true);
  WiFi.mode(WIFI_OFF);
  delay(10);
}

static bool begin_espnow_for_additional() {
  WiFi.mode(WIFI_STA);
  esp_wifi_set_channel(ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE);
  if (esp_now_init() != ESP_OK) {
    Serial.println("[ERROR] ESPNow init failed for additional sensors");
    return false;
  }
  // broadcast peer to reach any additional sensor using the shared header
  esp_now_peer_info_t peerInfo = {};
  memset(peerInfo.peer_addr, 0xFF, 6);
  peerInfo.channel = ESPNOW_CHANNEL;
   peerInfo.ifidx = WIFI_IF_STA; // parent is STA when requesting
  peerInfo.encrypt = false;
  esp_now_add_peer(&peerInfo);

  // add explicit peers for each additional sensor
  for (int i = 0; i < N_ADDITIONAL_SENSORS; ++i) {
    esp_now_peer_info_t peer = {};
    memcpy(peer.peer_addr, additional_sensor_mac_addrs[i], 6);
    peer.channel = ESPNOW_CHANNEL;
    peer.ifidx = WIFI_IF_STA;
    peer.encrypt = false;
    esp_err_t add_res = esp_now_add_peer(&peer);
    if (add_res != ESP_OK && add_res != ESP_ERR_ESPNOW_EXIST) {
      Serial.printf("[ERROR] add_peer failed (%d) for sensor %d\n", add_res, i);
    }
  }
  return true;
}

static void end_espnow_for_additional() {
  esp_now_deinit();
  WiFi.mode(WIFI_OFF);
  delay(10);
  init_wifi();
}

static void OnAdditionalDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len) {
  const int min_payload_bytes = N_SLAVE_HEADER + static_cast<int>(sizeof(float) * 2); // header + temp + humidity
  if (data_len < min_payload_bytes) {
    return;
  }

  float temperature_c = 0.0f;
  float humidity_pct = 0.0f;
  float pressure_hpa = FLT_MAX;
  memcpy(&temperature_c, data + N_SLAVE_HEADER, sizeof(float));
  memcpy(&humidity_pct, data + N_SLAVE_HEADER + sizeof(float), sizeof(float));
  if (data_len >= (int)sizeof(AdditionalSensorPacket)) {
    memcpy(&pressure_hpa, data + N_SLAVE_HEADER + sizeof(float) * 2, sizeof(float));
  }

  for (int i = 0; i < N_ADDITIONAL_SENSORS; ++i) {
    if (memcmp(data, additional_sensor_headers[i], N_SLAVE_HEADER) == 0) {
      g_additional_sensor_data[i].temperature = additional_sensor_available[i].temperature ? temperature_c : FLT_MAX;
      g_additional_sensor_data[i].humidity = additional_sensor_available[i].humidity ? humidity_pct : FLT_MAX;
      g_additional_sensor_data[i].pressure = (additional_sensor_available[i].pressure && data_len >= (int)sizeof(AdditionalSensorPacket)) ? pressure_hpa : FLT_MAX;
      g_additional_sensor_data[i].co2_concentration = FLT_MAX;
      g_additional_sensor_received[i] = true;
      g_additional_sensor_last_data[i] = g_additional_sensor_data[i];
      g_additional_sensor_last_ms[i] = millis();
      Serial.print("additional sensor ");
      Serial.print(i);
      Serial.print(" received: ");
      Serial.print("Temp: ");
      Serial.print(g_additional_sensor_data[i].temperature);
      Serial.print(" C, Humidity: ");
      Serial.print(g_additional_sensor_data[i].humidity);
      if (g_additional_sensor_data[i].pressure != FLT_MAX) {
        Serial.print(", Pressure: ");
        Serial.print(g_additional_sensor_data[i].pressure);
        Serial.print(" hPa");
      }
      Serial.println(" %");
    }
  }
}

void additional_sensors_request() {
  unsigned long now = millis();
  if (g_last_additional_request_ms != 0 && (now - g_last_additional_request_ms) < ADDITIONAL_SENSOR_REQUEST_INTERVAL_MS) {
    // recent data is still considered fresh; avoid WiFi reset thrash
    return;
  }

  for (int i = 0; i < N_ADDITIONAL_SENSORS; ++i) {
    g_additional_sensor_received[i] = false;
    g_additional_sensor_data[i] = SensorReading();
  }

  reset_wifi_for_espnow();
  if (!begin_espnow_for_additional()) {
    init_wifi();
    return;
  }

  esp_now_register_recv_cb(OnAdditionalDataRecv);

  for (int i = 0; i < N_ADDITIONAL_SENSORS; ++i) {
    Serial.print("request data to additional sensor ");
    Serial.println(i);
    esp_err_t send_res = esp_now_send(additional_sensor_mac_addrs[i], (uint8_t *)additional_sensor_headers[i], N_SLAVE_HEADER);
    if (send_res != ESP_OK) {
      Serial.printf("[WARN] esp_now_send failed (%d) to sensor %d\n", send_res, i);
    }
  }

  unsigned long start = millis();
  while (millis() - start < 1500) {
    delay(10);
  }

  end_espnow_for_additional();
  g_last_additional_request_ms = millis();
}

bool additional_sensor_received(int idx) {
  if (idx < 0 || idx >= N_ADDITIONAL_SENSORS) {
    return false;
  }
  if (g_additional_sensor_received[idx]) {
    return true;
  }

  unsigned long last = g_additional_sensor_last_ms[idx];
  if (last == 0) {
    return false; // never received
  }

  unsigned long now = millis();
  return (now - last) <= ADDITIONAL_SENSOR_MAX_AGE_MS;
}

SensorReading additional_sensor_data_get(int idx) {
  SensorReading empty;
  if (idx < 0 || idx >= N_ADDITIONAL_SENSORS) {
    return empty;
  }
  if (g_additional_sensor_received[idx]) {
    return g_additional_sensor_data[idx];
  }

  unsigned long last = g_additional_sensor_last_ms[idx];
  if (last == 0) {
    return empty;
  }

  unsigned long now = millis();
  if ((now - last) <= ADDITIONAL_SENSOR_MAX_AGE_MS) {
    Serial.print("[INFO] using cached additional sensor ");
    Serial.print(idx);
    Serial.print(" data from ");
    Serial.print((now - last) / 1000UL);
    Serial.println("s ago");
    return g_additional_sensor_last_data[idx];
  }

  return empty;
}
