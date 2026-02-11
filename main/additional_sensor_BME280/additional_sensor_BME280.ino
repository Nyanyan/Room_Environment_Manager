// additional temperature & humidity & pressure sensor BME280 using ESP32C3

#include <esp_now.h>
#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <cstring>
#include <float.h>
#include "token.h"

Adafruit_BME280 bme;
bool bme_ready = false;

#define CHANNEL 1

#define SENSOR_N_DATA_FOR_AVERAGE 25

struct __attribute__((packed)) SensorPacket {
  char header[N_SLAVE_HEADER];
  float temperature_c;
  float humidity_pct;
  float pressure_hpa;
};

double latest_temperature = 0.0;
double latest_humidity = 0.0;
double latest_pressure = 0.0;
static float temperature_samples[SENSOR_N_DATA_FOR_AVERAGE];
static float humidity_samples[SENSOR_N_DATA_FOR_AVERAGE];
static float pressure_samples[SENSOR_N_DATA_FOR_AVERAGE];

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


bool get_data_BME280(float *temperature, float *humidity, float *pressure) {
  if (!bme_ready) {
    Serial.println("[WARN] BME280 not ready");
    return false;
  }
  if (!bme.takeForcedMeasurement()) {
    Serial.println("[WARN] BME280 read failed");
    return false;
  }
  *temperature = bme.readTemperature();
  *humidity = bme.readHumidity();
  *pressure = bme.readPressure() / 100.0; // Pa -> hPa
  return true;
}

void get_temperature_humidity_pressure() {
  int success_reads = 0;
  for (int i = 0; i < SENSOR_N_DATA_FOR_AVERAGE; ++i) {
    Serial.print('=');
    if (get_data_BME280(&temperature_samples[success_reads], &humidity_samples[success_reads], &pressure_samples[success_reads])) {
      ++success_reads;
    } else {
      Serial.println("Failed to read from BME280, continue...");
    }
    delay(600);
  }

  if (success_reads == 0) {
    Serial.println("Completely Failed to read from BME280");
    return;
  }

  float temp_avg = compute_trimmed_mean(temperature_samples, success_reads);
  float hum_avg = compute_trimmed_mean(humidity_samples, success_reads);
  float pres_avg = compute_trimmed_mean(pressure_samples, success_reads);
  latest_temperature = temp_avg;
  latest_humidity = hum_avg;
  latest_pressure = pres_avg;

  Serial.print("Trimmed Avg Temp: ");
  Serial.print(latest_temperature);
  Serial.print(" C, Humidity: ");
  Serial.print(latest_humidity);
  Serial.print(" %, Pressure: ");
  Serial.print(latest_pressure);
  Serial.println(" hPa");
}


void setup() {
  Serial.begin(115200);
  delay(200);

  // Initialize I2C on ESP32-C3 (default: SDA=D4, SCL=D5)
  Wire.begin();
  Wire.setTimeout(2000);

  if (!bme.begin(0x76)) {
    Serial.println("Failed to find BME280 sensor");
    while (true) {
      delay(1000);
    }
  }

  Serial.println("start BME280 sensor");

  // Force mode with moderate oversampling for stability
  bme.setSampling(Adafruit_BME280::MODE_FORCED,
                  Adafruit_BME280::SAMPLING_X4,   // temperature
                  Adafruit_BME280::SAMPLING_X4,   // pressure
                  Adafruit_BME280::SAMPLING_X4,   // humidity
                  Adafruit_BME280::FILTER_X4);
  bme_ready = true;

  get_temperature_humidity_pressure();


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
    packet.pressure_hpa = (float)latest_pressure;     // already trimmed mean

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
      Serial.println("Sent temp/humidity/pressure to parent");
    } else {
      Serial.printf("Send failed: %d\n", result);
    }
  }
}

void loop() {
  get_temperature_humidity_pressure();
}
