#include <Wire.h>
#include <MHZ19_uart.h> // https://github.com/nara256/mhz19_uart
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <float.h>
#include "Adafruit_BME680.h"
#include "sensors.h"
#include "token.h"

// Avoid rare I2C lockups by enforcing a timeout on transactions.
constexpr uint16_t I2C_TIMEOUT_MS = 2000;

// CO2 sensor
MHZ19_uart mhz19;

// Pressure Sensor
Adafruit_BME680 bme;


void init_BME680(bool show_log){
  bme.begin();
  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  bme.setGasHeater(320, 150);
  if (show_log) {
    display_print(0, 2, "[I] BME680 initialized");
  }
}


void init_sensors(){
  Wire.begin();
  Wire.setTimeout(I2C_TIMEOUT_MS);
  init_BME680(true);
}



// Temperature / Humidity / Pressure (BME680)
bool get_data_BME680(float *temperature, float *humidity, float *pressure) {
  if (!bme.performReading()) {
    Serial.println("[WARN] BME680 read failed");
    return false;
  }
  *temperature = bme.temperature;
  *humidity = bme.humidity;
  *pressure = bme.pressure / 100.0;
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
  Sensor_data sensor_data; // initialized with FLT_MAX sentinels

  float temperatures[SENSOR_N_DATA_FOR_AVERAGE], humidities[SENSOR_N_DATA_FOR_AVERAGE], pressures[SENSOR_N_DATA_FOR_AVERAGE], co2_concentrations[SENSOR_N_DATA_FOR_AVERAGE];
  int n_temperature = 0;
  int n_humidity = 0;
  int n_pressure = 0;
  int n_co2_concentration = 0;
  for (int i = 0; i < SENSOR_N_DATA_FOR_AVERAGE; ++i){
    Serial.print("=");
    bool ok_bme680 = get_data_BME680(&temperatures[n_temperature], &humidities[n_humidity], &pressures[n_pressure]);
    if (ok_bme680) {
      ++n_temperature;
      ++n_humidity;
      ++n_pressure;
    }
    co2_concentrations[n_co2_concentration++] = mhz19.getCO2PPM();
  }
  Serial.println("");

  // Parent (local) sensor data
  sensor_data.parent.temperature = compute_trimmed_mean(temperatures, n_temperature);
  sensor_data.parent.humidity = compute_trimmed_mean(humidities, n_humidity);
  sensor_data.parent.pressure = compute_trimmed_mean(pressures, n_pressure);
  sensor_data.parent.co2_concentration = compute_trimmed_mean(co2_concentrations, n_co2_concentration);

  Serial.print("Sensor data: ");
  Serial.print("Temp: ");
  Serial.print(sensor_data.parent.temperature);
  Serial.print("*C ");
  Serial.print("Hum: ");
  Serial.print(sensor_data.parent.humidity);
  Serial.print("% ");
  Serial.print("Prs: ");
  Serial.print(sensor_data.parent.pressure);
  Serial.print("hPa ");
  Serial.print("CO2: ");
  Serial.print(sensor_data.parent.co2_concentration);
  Serial.print("ppm ");
  Serial.println("");
  return sensor_data;
}
