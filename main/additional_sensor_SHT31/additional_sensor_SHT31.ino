// Additional temperature & humidity sensor SHT31 using ESP32C3

#include <esp_now.h>
#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_SHT31.h>
#include <cstring>
#include <float.h>
#include <math.h>
#include "token.h"

Adafruit_SHT31 sht31;

#define CHANNEL 1

#define SENSOR_N_DATA_FOR_AVERAGE 25

struct __attribute__((packed)) SensorPacket {
  char header[N_SLAVE_HEADER];
  float temperature_c;
  float humidity_pct;
};

double latest_temperature = 0.0;
double latest_humidity = 0.0;
static float temperature_samples[SENSOR_N_DATA_FOR_AVERAGE];
static float humidity_samples[SENSOR_N_DATA_FOR_AVERAGE];

// Compute trimmed mean after sorting and dropping 5% high/low outliers.
float compute_trimmed_mean(const float *samples, int count) {
  if (count <= 0) {
    return FLT_MAX; // signal error when no samples are available
  }
  float sorted[SENSOR_N_DATA_FOR_AVERAGE];
  for (int i = 0; i < count; ++i) {
    sorted[i] = samples[i];
  }

  // Insertion sort is efficient enough for small, fixed-size arrays.
  for (int i = 1; i < count; ++i) {
    float key = sorted[i];
    int j = i - 1;
    while (j >= 0 && sorted[j] > key) {
      sorted[j + 1] = sorted[j];
      --j;
    }
    sorted[j + 1] = key;
  }

  int trim = (count * 5) / 100; // drop 5% from each end
  if (trim * 2 >= count) {
    trim = max(0, count / 2 - 1); // ensure at least one sample remains
  }

  int start = trim;
  int end = count - trim;
  int used = end - start;
  if (used <= 0) {
    return sorted[count / 2]; // fallback to median if trimming leaves none
  }

  float sum = 0.0;
  for (int i = start; i < end; ++i) {
    sum += sorted[i];
  }
  return sum / used;
}


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
  const char *SSID = "additional_sensor";
  bool result = WiFi.softAP(SSID, "additional_sensor_password", CHANNEL, 0);
  if (!result) {
    Serial.println("AP Config failed.");
  } else {
    Serial.println("AP Config Success. Broadcasting with AP: " + String(SSID));
    Serial.print("AP CHANNEL "); Serial.println(WiFi.channel());
  }
}


void get_temperature_humidity() {
  int success_reads = 0;
  for (int i = 0; i < SENSOR_N_DATA_FOR_AVERAGE; ++i) {
    Serial.print('=');
    float temperature = sht31.readTemperature();
    float humidity = sht31.readHumidity();
    if (!isnan(temperature) && !isnan(humidity)) {
      temperature_samples[success_reads] = temperature;
      humidity_samples[success_reads] = humidity;
      ++success_reads;
    } else {
      Serial.println("Failed to read from SHT31, continue...");
    }
    delay(500);
  }

  if (success_reads == 0) {
    Serial.println("Completely Failed to read from SHT31");
    return;
  }

  float temp_avg = compute_trimmed_mean(temperature_samples, success_reads);
  float hum_avg = compute_trimmed_mean(humidity_samples, success_reads);
  latest_temperature = temp_avg;
  latest_humidity = hum_avg;

  Serial.print("Trimmed Avg Temp: ");
  Serial.print(latest_temperature);
  Serial.print(" C, Humidity: ");
  Serial.print(latest_humidity);
  Serial.println(" %");
}


void setup() {
  Serial.begin(115200);
  delay(200);

  // Initialize I2C on ESP32-C3 (default: SDA=D4, SCL=D5)
  Wire.begin();

  if (!sht31.begin(0x45)) {
    Serial.println("Failed to find SHT31 sensor");
    while (true) {
      delay(1000);
    }
  }

  Serial.println("start SHT31 sensor");

  // Disable heater to avoid self-heating during continuous readings.
  sht31.heater(false);

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
    SensorPacket packet = {};
    memcpy(packet.header, additional_sensor_header, N_SLAVE_HEADER);
    packet.temperature_c = (float)latest_temperature; // already trimmed mean
    packet.humidity_pct = (float)latest_humidity;     // already trimmed mean

    // ensure the sender is added as peer so we can reply
    if (!esp_now_is_peer_exist(mac_addr)) {
      esp_now_peer_info_t peerInfo = {};
      memcpy(peerInfo.peer_addr, mac_addr, 6);
      peerInfo.channel = CHANNEL;
      peerInfo.ifidx = WIFI_IF_AP; // we are in AP mode
      peerInfo.encrypt = false;
      esp_err_t addStatus = esp_now_add_peer(&peerInfo);
      if (addStatus != ESP_OK) {
        Serial.printf("Add peer failed: %d\n", addStatus);
      }
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
}
