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

// Per-metric weights for parent sensor
const double sensor_weight_parent_temperature = 0.1;
const double sensor_weight_parent_humidity =    0.1;
const double sensor_weight_parent_pressure =    1.0;
const double sensor_weight_parent_co2 =         1.0;

// Per-metric weights for each additional sensor (index-aligned to token.h entries)
const double sensor_weights_additional_temperature[N_ADDITIONAL_SENSORS] =  {1.0, 1.0, 1.0};
const double sensor_weights_additional_humidity[N_ADDITIONAL_SENSORS] =     {1.0, 1.0, 1.0};
const double sensor_weights_additional_pressure[N_ADDITIONAL_SENSORS] =     {0.0, 1.0, 0.0};
const double sensor_weights_additional_co2[N_ADDITIONAL_SENSORS] =          {0.0, 0.0, 0.0};


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
  init_BME680(true);
  init_MHZ19C(true);
  additional_sensors_request();
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


// Helper to log sensor readings with graceful handling for missing values
void print_sensor_values(float temperature, float humidity, float pressure, float co2_concentration) {
  auto print_value = [](const char *name, float value) {
    Serial.print(name);
    Serial.print(": ");
    if (value == FLT_MAX) {
      Serial.print("N/A");
    } else {
      Serial.print(value);
    }
    Serial.print(" ");
  };

  print_value("Temp", temperature);
  print_value("Hum", humidity);
  print_value("Prs", pressure);
  print_value("CO2", co2_concentration);
  Serial.println("");
}



struct Sensor_data get_sensor_data(){
  // fetch latest additional sensor data via ESP-NOW
  additional_sensors_request();

  Sensor_data sensor_data; // initialized with FLT_MAX sentinels

  float temperatures_bme[SENSOR_N_DATA_FOR_AVERAGE], humidities_bme[SENSOR_N_DATA_FOR_AVERAGE], pressures_bme[SENSOR_N_DATA_FOR_AVERAGE];
  float co2_concentrations[SENSOR_N_DATA_FOR_AVERAGE];
  int n_temperature_bme = 0;
  int n_humidity_bme = 0;
  int n_pressure_bme = 0;
  int n_co2_concentration = 0;
  for (int i = 0; i < SENSOR_N_DATA_FOR_AVERAGE; ++i){
    Serial.print("=");
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
  float temp_bme = compute_trimmed_mean(temperatures_bme, n_temperature_bme);
  float hum_bme = compute_trimmed_mean(humidities_bme, n_humidity_bme);

  sensor_data.parent.temperature = temp_bme;
  sensor_data.parent.humidity = hum_bme;
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
  accumulate_if_valid(sensor_data.parent.temperature, sensor_weight_parent_temperature, representative_sum.temperature, weight_sum.temperature);
  accumulate_if_valid(sensor_data.parent.humidity, sensor_weight_parent_humidity, representative_sum.humidity, weight_sum.humidity);
  accumulate_if_valid(sensor_data.parent.pressure, sensor_weight_parent_pressure, representative_sum.pressure, weight_sum.pressure);
  accumulate_if_valid(sensor_data.parent.co2_concentration, sensor_weight_parent_co2, representative_sum.co2_concentration, weight_sum.co2_concentration);

  for (int i = 0; i < N_ADDITIONAL_SENSORS; ++i) {
    if (additional_sensor_received(i)) {
      SensorReading data = additional_sensor_data_get(i);
      sensor_data.additional[i] = data;
      accumulate_if_valid(data.temperature, sensor_weights_additional_temperature[i], representative_sum.temperature, weight_sum.temperature);
      accumulate_if_valid(data.humidity, sensor_weights_additional_humidity[i], representative_sum.humidity, weight_sum.humidity);
      accumulate_if_valid(data.pressure, sensor_weights_additional_pressure[i], representative_sum.pressure, weight_sum.pressure);
      accumulate_if_valid(data.co2_concentration, sensor_weights_additional_co2[i], representative_sum.co2_concentration, weight_sum.co2_concentration);
    }
  }

  sensor_data.representative.temperature = (weight_sum.temperature > 0.0f) ? representative_sum.temperature / weight_sum.temperature : FLT_MAX;
  sensor_data.representative.humidity = (weight_sum.humidity > 0.0f) ? representative_sum.humidity / weight_sum.humidity : FLT_MAX;
  sensor_data.representative.pressure = (weight_sum.pressure > 0.0f) ? representative_sum.pressure / weight_sum.pressure : FLT_MAX;
  sensor_data.representative.co2_concentration = (weight_sum.co2_concentration > 0.0f) ? representative_sum.co2_concentration / weight_sum.co2_concentration : FLT_MAX;

  Serial.println("Parent and additional sensor data:");
  Serial.print("Parent ");
  print_sensor_values(sensor_data.parent.temperature, sensor_data.parent.humidity, sensor_data.parent.pressure, sensor_data.parent.co2_concentration);
  for (int i = 0; i < N_ADDITIONAL_SENSORS; ++i) {
    Serial.print("Child");
    Serial.print(i);
    Serial.print(" ");
    print_sensor_values(sensor_data.additional[i].temperature, sensor_data.additional[i].humidity, sensor_data.additional[i].pressure, sensor_data.additional[i].co2_concentration);
  }

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















