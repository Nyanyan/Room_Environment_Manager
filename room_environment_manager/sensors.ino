#include <Wire.h>
#include <MHZ19_uart.h> // https://github.com/nara256/mhz19_uart
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <float.h>
#include "Adafruit_BME680.h"
#include "sensors.h"

// Avoid rare I2C lockups by enforcing a timeout on transactions.
constexpr uint16_t I2C_TIMEOUT_MS = 2000;

// CO2 sensor
MHZ19_uart mhz19;

// Pressure Sensor
Adafruit_BME680 bme;


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



// Pressure
float get_data_BME680() {
  bme.performReading();
  return bme.pressure / 100.0;
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
  float temperatures[SENSOR_N_DATA_FOR_AVERAGE], humidities[SENSOR_N_DATA_FOR_AVERAGE], pressures[SENSOR_N_DATA_FOR_AVERAGE], co2_concentrations[SENSOR_N_DATA_FOR_AVERAGE];
  int n_temperature = 0;
  int n_humidity = 0;
  int n_pressure = 0;
  int n_co2_concentration = 0;
  for (int i = 0; i < SENSOR_N_DATA_FOR_AVERAGE; ++i){
    Serial.print("=");
    bool ok_sht31 = get_data_SHT31(&temperatures[n_temperature], &humidities[n_humidity]);
    if (ok_sht31) {
      ++n_temperature;
      ++n_humidity;
    }
    pressures[n_pressure++] = get_data_BME680();
    co2_concentrations[n_co2_concentration++] = mhz19.getCO2PPM();
  }
  Serial.println("");
  Sensor_data sensor_data;
  sensor_data.temperature = compute_trimmed_mean(temperatures, n_temperature);
  sensor_data.humidity = compute_trimmed_mean(humidities, n_humidity);
  sensor_data.pressure = compute_trimmed_mean(pressures, n_pressure);
  sensor_data.co2_concentration = compute_trimmed_mean(co2_concentrations, n_co2_concentration);

  Serial.print("Temp: ");
  Serial.print(sensor_data.temperature);
  Serial.print("*C ");
  Serial.print("Hum: ");
  Serial.print(sensor_data.humidity);
  Serial.print("% ");
  Serial.print("Prs: ");
  Serial.print(sensor_data.pressure);
  Serial.print("hPa ");
  Serial.print("CO2: ");
  Serial.print(sensor_data.co2_concentration);
  Serial.print("ppm ");
  Serial.println("");
  return sensor_data;
}

float sensor_calc_thi(float temperature, float humidity) {
  return 0.81 * temperature + 0.01 * humidity * (0.99 * temperature - 14.3) + 46.3;
}






