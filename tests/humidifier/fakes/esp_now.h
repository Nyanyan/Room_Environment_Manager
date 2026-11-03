#pragma once
#include "Arduino.h"
using esp_err_t = int;
constexpr int ESP_OK = 0, ESP_ERR_ESPNOW_EXIST = 1;
struct esp_now_peer_info_t { uint8_t peer_addr[6]; int ifidx; int channel; bool encrypt; };
inline std::vector<std::vector<uint8_t>> fake_packets;
inline int fake_peer_result = ESP_OK;
inline int esp_now_init() { return ESP_OK; }
inline int esp_now_deinit() { return ESP_OK; }
inline int esp_now_add_peer(const esp_now_peer_info_t *) { return fake_peer_result; }
inline int esp_now_register_recv_cb(void (*)(const uint8_t *, const uint8_t *, int)) { return ESP_OK; }
inline int esp_now_send(const uint8_t *, const uint8_t *data, size_t length) {
  fake_packets.emplace_back(data, data + length);
  return ESP_OK;
}
