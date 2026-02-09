#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <cstring>
#include "additional_sensors.h"

// Forward declaration from slack.ino
void init_wifi();

// ESP-NOW channel shared with the additional sensor nodes.
static const uint8_t ESPNOW_CHANNEL = 1;

struct AdditionalSensorPacket {
  char header[N_SLAVE_HEADER];
  float temperature_c;
  float humidity_pct;
};

static Sensor_data g_additional_sensor_data[N_ADDITIONAL_SENSORS];
static bool g_additional_sensor_received[N_ADDITIONAL_SENSORS];

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
  peerInfo.encrypt = false;
  esp_now_add_peer(&peerInfo);
  return true;
}

static void end_espnow_for_additional() {
  esp_now_deinit();
  WiFi.mode(WIFI_OFF);
  delay(10);
  init_wifi();
}

static void OnAdditionalDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len) {
  if (data_len < (int)sizeof(AdditionalSensorPacket)) {
    return;
  }
  const AdditionalSensorPacket *packet = reinterpret_cast<const AdditionalSensorPacket *>(data);
  for (int i = 0; i < N_ADDITIONAL_SENSORS; ++i) {
    if (memcmp(packet->header, additional_sensor_headers[i], N_SLAVE_HEADER) == 0) {
      g_additional_sensor_data[i].temperature = packet->temperature_c;
      g_additional_sensor_data[i].humidity = packet->humidity_pct;
      g_additional_sensor_data[i].pressure = 0.0f;
      g_additional_sensor_data[i].co2_concentration = 0.0f;
      g_additional_sensor_received[i] = true;
    }
  }
}

void additional_sensors_request() {
  for (int i = 0; i < N_ADDITIONAL_SENSORS; ++i) {
    g_additional_sensor_received[i] = false;
    g_additional_sensor_data[i].temperature = 0.0f;
    g_additional_sensor_data[i].humidity = 0.0f;
    g_additional_sensor_data[i].pressure = 0.0f;
    g_additional_sensor_data[i].co2_concentration = 0.0f;
  }

  reset_wifi_for_espnow();
  if (!begin_espnow_for_additional()) {
    init_wifi();
    return;
  }

  esp_now_register_recv_cb(OnAdditionalDataRecv);

  const uint8_t broadcast_addr[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  for (int i = 0; i < N_ADDITIONAL_SENSORS; ++i) {
    esp_now_send(broadcast_addr, (uint8_t *)additional_sensor_headers[i], N_SLAVE_HEADER);
  }

  unsigned long start = millis();
  while (millis() - start < 500) {
    delay(10);
  }

  end_espnow_for_additional();
}

bool additional_sensor_received(int idx) {
  if (idx < 0 || idx >= N_ADDITIONAL_SENSORS) {
    return false;
  }
  return g_additional_sensor_received[idx];
}

Sensor_data additional_sensor_data_get(int idx) {
  Sensor_data empty = {0, 0, 0, 0};
  if (idx < 0 || idx >= N_ADDITIONAL_SENSORS) {
    return empty;
  }
  return g_additional_sensor_data[idx];
}
