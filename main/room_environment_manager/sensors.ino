#include <Wire.h>
#include <MHZ19_uart.h> // https://github.com/nara256/mhz19_uart
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <float.h>
#include "Adafruit_BME680.h"
#include "sensors.h"
#include "token.h"
#include "additional_sensors.h"

// Avoid rare I2C lockups by enforcing a timeout on transactions.
constexpr uint16_t I2C_TIMEOUT_MS = 2000;

// CO2 sensor
MHZ19_uart mhz19;

// Pressure Sensor
Adafruit_BME680 bme;

const double sensor_weight_parent = 0.5;
const double sensor_weights_additional[N_ADDITIONAL_SENSORS] = {1.0, 1.5};


void init_SHT31(bool show_log){
  Wire.beginTransmission(SHT31_ADDR);
  Wire.write(SHT31_SOFT_RESET_MSB);
  Wire.write(SHT31_SOFT_RESET_LSB);
  Wire.endTransmission();
  delay(500);
  Wire.beginTransmission(SHT31_ADDR);
  Wire.write(SHT31_CLEAR_STATUS_REGISTER_MSB);
  Wire.write(SHT31_CLEAR_STATUS_REGISTER_LSB);
  Wire.endTransmission();
  delay(500);
  Serial.println("Temperature & Humidity Sensor Initialized");
  if (show_log) {
    display_print(0, 1, "[I] SHT31 TH");
  }
}


void init_BME680(bool show_log){
  bme.begin();
  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  bme.setGasHeater(320, 150);
  if (show_log) {
    display_print(0, 2, "[I] BME680 Pressure");
  }
}


void init_MHZ19C(bool show_log){
  mhz19.begin(MHZ19C_RX_PIN, MHZ19C_TX_PIN);
  mhz19.setAutoCalibration(true);
  delay(10000);
  Serial.println("CO2 Sensor Initialized");
  if (show_log) {
    display_print(0, 3, "[I] MH-Z19C CO2");
  }
}



void init_sensors(){
  Wire.begin();
  Wire.setTimeout(I2C_TIMEOUT_MS);
  init_SHT31(true);
  init_BME680(true);
  init_MHZ19C(true);
  additional_sensors_request();
}



// temperature & humidity
bool get_data_SHT31(float *temperature, float *humidity) {
  unsigned int buf[6];
  Wire.beginTransmission(SHT31_ADDR);
  Wire.write(SHT31_SINGLE_SHOT_HIGH_MSB);
  Wire.write(SHT31_SINGLE_SHOT_HIGH_LSB);
  Wire.endTransmission();
  delay(300);
  int n_bytes = Wire.requestFrom(SHT31_ADDR, 6);
  if (n_bytes != 6) {
    Serial.println("[WARN] SHT31 read timed out");
    init_SHT31(false);
    delay(1000);
    return false;
  }
  for (int i = 0; i < 6; ++i){
    buf[i] = Wire.read();
  }
  uint32_t t = (buf[0] << 8) | buf[1];
  *temperature = (float)t * 175 / 65535.0 - 45.0;
  uint32_t h = (buf[3] << 8) | buf[4];
  *humidity = (float)(h) / 65535.0 * 100.0;
  return true;
}



// Temperature / Humidity / Pressure (BME680)
bool get_data_BME680(float *temperature, float *humidity, float *pressure) {
  if (!bme.performReading()) {
    Serial.println("[WARN] BME680 read failed");
    return false;
  }
  *temperature = bme.temperature;
  *humidity = bme.humidity;
  *pressure = bme.pressure / 100.0; // Pa -> hPa
  return true;
}


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



struct Sensor_data get_sensor_data(){
  // fetch latest additional sensor data via ESP-NOW
  additional_sensors_request();

  Sensor_data sensor_data; // initialized with FLT_MAX sentinels

  float temperatures_sht[SENSOR_N_DATA_FOR_AVERAGE], humidities_sht[SENSOR_N_DATA_FOR_AVERAGE];
  float temperatures_bme[SENSOR_N_DATA_FOR_AVERAGE], humidities_bme[SENSOR_N_DATA_FOR_AVERAGE], pressures_bme[SENSOR_N_DATA_FOR_AVERAGE];
  float co2_concentrations[SENSOR_N_DATA_FOR_AVERAGE];
  int n_temperature_sht = 0;
  int n_humidity_sht = 0;
  int n_temperature_bme = 0;
  int n_humidity_bme = 0;
  int n_pressure_bme = 0;
  int n_co2_concentration = 0;
  for (int i = 0; i < SENSOR_N_DATA_FOR_AVERAGE; ++i){
    Serial.print("=");
    bool ok_sht31 = get_data_SHT31(&temperatures_sht[n_temperature_sht], &humidities_sht[n_humidity_sht]);
    if (ok_sht31) {
      ++n_temperature_sht;
      ++n_humidity_sht;
    }

    float t_bme = 0.0f, h_bme = 0.0f, p_bme = 0.0f;
    bool ok_bme680 = get_data_BME680(&t_bme, &h_bme, &p_bme);
    if (ok_bme680) {
      temperatures_bme[n_temperature_bme++] = t_bme;
      humidities_bme[n_humidity_bme++] = h_bme;
      pressures_bme[n_pressure_bme++] = p_bme;
    }
    co2_concentrations[n_co2_concentration++] = mhz19.getCO2PPM();
  }
  Serial.println("");

  // Parent (local) sensor data
  auto average_two = [](float a, float b) {
    int count = 0;
    float sum = 0.0f;
    if (a != FLT_MAX) { sum += a; ++count; }
    if (b != FLT_MAX) { sum += b; ++count; }
    return (count > 0) ? sum / count : FLT_MAX;
  };

  float temp_sht = compute_trimmed_mean(temperatures_sht, n_temperature_sht);
  float hum_sht = compute_trimmed_mean(humidities_sht, n_humidity_sht);
  float temp_bme = compute_trimmed_mean(temperatures_bme, n_temperature_bme);
  float hum_bme = compute_trimmed_mean(humidities_bme, n_humidity_bme);

  sensor_data.parent.temperature = average_two(temp_sht, temp_bme);
  sensor_data.parent.humidity = average_two(hum_sht, hum_bme);
  sensor_data.parent.pressure = compute_trimmed_mean(pressures_bme, n_pressure_bme);
  sensor_data.parent.co2_concentration = compute_trimmed_mean(co2_concentrations, n_co2_concentration);

  // Representative (weighted) data for control/graphs
  SensorReading representative_sum;
  SensorReading weight_sum;
  representative_sum.temperature = 0.0f;
  representative_sum.humidity = 0.0f;
  representative_sum.pressure = 0.0f;
  representative_sum.co2_concentration = 0.0f;
  weight_sum.temperature = 0.0f;
  weight_sum.humidity = 0.0f;
  weight_sum.pressure = 0.0f;
  weight_sum.co2_concentration = 0.0f;

  auto accumulate_if_valid = [&](float value, float weight, float &acc, float &w_acc) {
    if (value != FLT_MAX) {
      acc += value * weight;
      w_acc += weight;
    }
  };

  // base with parent sensor contribution
  accumulate_if_valid(sensor_data.parent.temperature, sensor_weight_parent, representative_sum.temperature, weight_sum.temperature);
  accumulate_if_valid(sensor_data.parent.humidity, sensor_weight_parent, representative_sum.humidity, weight_sum.humidity);
  accumulate_if_valid(sensor_data.parent.pressure, sensor_weight_parent, representative_sum.pressure, weight_sum.pressure);
  accumulate_if_valid(sensor_data.parent.co2_concentration, sensor_weight_parent, representative_sum.co2_concentration, weight_sum.co2_concentration);

  for (int i = 0; i < N_ADDITIONAL_SENSORS; ++i) {
    if (additional_sensor_received(i)) {
      SensorReading data = additional_sensor_data_get(i);
      sensor_data.additional[i] = data;
      accumulate_if_valid(data.temperature, sensor_weights_additional[i], representative_sum.temperature, weight_sum.temperature);
      accumulate_if_valid(data.humidity, sensor_weights_additional[i], representative_sum.humidity, weight_sum.humidity);
      accumulate_if_valid(data.pressure, sensor_weights_additional[i], representative_sum.pressure, weight_sum.pressure);
      accumulate_if_valid(data.co2_concentration, sensor_weights_additional[i], representative_sum.co2_concentration, weight_sum.co2_concentration);
    }
  }

  sensor_data.representative.temperature = (weight_sum.temperature > 0.0f) ? representative_sum.temperature / weight_sum.temperature : FLT_MAX;
  sensor_data.representative.humidity = (weight_sum.humidity > 0.0f) ? representative_sum.humidity / weight_sum.humidity : FLT_MAX;
  sensor_data.representative.pressure = (weight_sum.pressure > 0.0f) ? representative_sum.pressure / weight_sum.pressure : FLT_MAX;
  sensor_data.representative.co2_concentration = (weight_sum.co2_concentration > 0.0f) ? representative_sum.co2_concentration / weight_sum.co2_concentration : FLT_MAX;

  Serial.print("Representative data: ");
  Serial.print("Temp: ");
  Serial.print(sensor_data.representative.temperature);
  Serial.print("*C ");
  Serial.print("Hum: ");
  Serial.print(sensor_data.representative.humidity);
  Serial.print("% ");
  Serial.print("Prs: ");
  Serial.print(sensor_data.representative.pressure);
  Serial.print("hPa ");
  Serial.print("CO2: ");
  Serial.print(sensor_data.representative.co2_concentration);
  Serial.print("ppm ");
  Serial.println("");
  return sensor_data;
}

float sensor_calc_thi(float temperature, float humidity) {
  return 0.81 * temperature + 0.01 * humidity * (0.99 * temperature - 14.3) + 46.3;
}






