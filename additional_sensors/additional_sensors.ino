// additional temperature & humidity sensor SHT40I using ESP32C3

#include <esp_now.h>
#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_SHT4x.h>
#include <cstring>
#include "token.h"

Adafruit_SHT4x sht4;

#define CHANNEL 1

#define SENSOR_N_DATA_FOR_AVERAGE 25

struct SensorPacket {
  char header[N_SLAVE_HEADER];
  float temperature_c;
  float humidity_pct;
};

double latest_temperature = 0.0;
double latest_humidity = 0.0;

bool is_correct_header(const uint8_t* data) {
  for (int i = 0; i < N_SLAVE_HEADER; ++i) {
    if (data[i] != additional_sensor_header[i]) {
      return false;
    }
  }
  return true;
}


// Init ESP Now with fallback
void InitESPNow() {
  WiFi.disconnect();
  if (esp_now_init() == ESP_OK) {
    Serial.println("ESPNow Init Success");
  }
  else {
    Serial.println("ESPNow Init Failed");
    // Retry InitESPNow, add a counte and then restart?
    // InitESPNow();
    // or Simply Restart
    ESP.restart();
  }
}

// config AP SSID
void configDeviceAP() {
  const char *SSID = "Slave_1";
  bool result = WiFi.softAP(SSID, "Slave_1_Password", CHANNEL, 0);
  if (!result) {
    Serial.println("AP Config failed.");
  } else {
    Serial.println("AP Config Success. Broadcasting with AP: " + String(SSID));
    Serial.print("AP CHANNEL "); Serial.println(WiFi.channel());
  }
}


void get_temperature_humidity() {
  sensors_event_t humidity_event, temp_event;
  if (sht4.getEvent(&humidity_event, &temp_event)) {
    Serial.print("Temp: ");
    Serial.print(temp_event.temperature);
    Serial.print(" C, Humidity: ");
    Serial.print(humidity_event.relative_humidity);
    Serial.println(" %");
    latest_temperature = temp_event.temperature;
    latest_humidity = humidity_event.relative_humidity;
  } else {
    Serial.println("Failed to read from SHT4x");
  }
}


void setup() {
  Serial.begin(115200);
  delay(200);

  // Initialize I2C on ESP32-C3 with SDA=D4, SCL=D5.
  Wire.begin();

  if (!sht4.begin()) {
    Serial.println("Failed to find SHT4x sensor");
    while (true) {
      delay(1000);
    }
  }

  Serial.println("start SHT4x sensor");

  // Use highest precision, no heater for continuous readings.
  sht4.setPrecision(SHT4X_HIGH_PRECISION);
  sht4.setHeater(SHT4X_NO_HEATER);

  get_temperature_humidity();


  //Set device in AP mode to begin with
  WiFi.mode(WIFI_AP);
  // configure device AP mode
  configDeviceAP();
  // This is the mac address of the Slave in AP Mode
  Serial.print("AP MAC: "); Serial.println(WiFi.softAPmacAddress());
  // Init ESPNow with a fallback logic
  InitESPNow();
  // Once ESPNow is successfully Init, we will register for recv CB to
  // get recv packer info.
  esp_now_register_recv_cb(OnDataRecv);
}



// callback when data is recv from Master
void OnDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len) {
  Serial.println("received");
  if (is_correct_header(data)){ // check header
    SensorPacket packet;
    memcpy(packet.header, additional_sensor_header, N_SLAVE_HEADER);
    packet.temperature_c = (float)latest_temperature;
    packet.humidity_pct = (float)latest_humidity;

    // ensure the sender is added as peer so we can reply
    if (!esp_now_is_peer_exist(mac_addr)) {
      esp_now_peer_info_t peerInfo = {};
      memcpy(peerInfo.peer_addr, mac_addr, 6);
      peerInfo.channel = CHANNEL;
      peerInfo.encrypt = false;
      esp_now_add_peer(&peerInfo);
    }

    esp_err_t result = esp_now_send(mac_addr, reinterpret_cast<uint8_t*>(&packet), sizeof(packet));
    if (result == ESP_OK) {
      Serial.println("Sent temp/humidity to parent");
    } else {
      Serial.printf("Send failed: %d\n", result);
    }
  }
}

void loop() {
  get_temperature_humidity();
  delay(2000);
}
