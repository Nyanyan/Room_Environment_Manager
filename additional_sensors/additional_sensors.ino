// additional temperature & humidity sensor SHT40I using ESP32C3

#include <esp_now.h>
#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_SHT4x.h>
#include "token.h"

Adafruit_SHT4x sht4;

#define CHANNEL 1


double temperature, humidity;


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
  sensors_event_t humidity, temp;
  if (sht4.getEvent(&humidity, &temp)) {
    Serial.print("Temp: ");
    Serial.print(temp.temperature);
    Serial.print(" C, Humidity: ");
    Serial.print(humidity.relative_humidity);
    Serial.println(" %");
    temperature = temp.temperature;
    humidity = humidity.relative_humidity;
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
    // ここで最新の気温と湿度を親に渡す
  }
}

void loop() {
  get_temperature_humidity();
}
