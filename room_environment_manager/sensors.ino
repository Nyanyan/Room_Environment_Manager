#include <Wire.h>
#include <MHZ19_uart.h> // https://github.com/nara256/mhz19_uart
#include <SPI.h>
#include <Adafruit_Sensor.h>
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



struct Sensor_data get_sensor_data(){
  float temperatures[SENSOR_N_DATA_FOR_AVERAGE], humidities[SENSOR_N_DATA_FOR_AVERAGE], pressures[SENSOR_N_DATA_FOR_AVERAGE], co2_concentrations[SENSOR_N_DATA_FOR_AVERAGE];
  float min_temperature = 10000.0, min_humidity = 10000.0, min_pressure = 10000.0, min_co2_concentration = 10000.0;
  float max_temperature = -10000.0, max_humidity = -10000.0, max_pressure = -10000.0, max_co2_concentration = -10000.0;
  static float last_temperature = 25.0;
  static float last_humidity = 50.0;
  for (int i = 0; i < SENSOR_N_DATA_FOR_AVERAGE; ++i){
    Serial.print("=");
    bool ok_sht31 = get_data_SHT31(&temperatures[i], &humidities[i]);
    if (!ok_sht31) {
      // Reuse last known good data to avoid blocking the loop forever.
      temperatures[i] = last_temperature;
      humidities[i] = last_humidity;
    } else {
      last_temperature = temperatures[i];
      last_humidity = humidities[i];
    }
    pressures[i] = get_data_BME680();
    co2_concentrations[i] = mhz19.getCO2PPM();
    min_temperature = min(min_temperature, temperatures[i]);
    min_humidity = min(min_humidity, humidities[i]);
    min_pressure = min(min_pressure, pressures[i]);
    min_co2_concentration = min(min_co2_concentration, co2_concentrations[i]);
    max_temperature = max(max_temperature, temperatures[i]);
    max_humidity = max(max_humidity, humidities[i]);
    max_pressure = max(max_pressure, pressures[i]);
    max_co2_concentration = max(max_co2_concentration, co2_concentrations[i]);
  }
  Serial.println("");
  Sensor_data sensor_data;
  sensor_data.temperature = 0.0;
  sensor_data.humidity = 0.0;
  sensor_data.pressure = 0.0;
  sensor_data.co2_concentration = 0.0;
  for (int i = 0; i < SENSOR_N_DATA_FOR_AVERAGE; ++i){
    sensor_data.temperature += temperatures[i];
    sensor_data.humidity += humidities[i];
    sensor_data.pressure += pressures[i];
    sensor_data.co2_concentration += co2_concentrations[i];
  }
  sensor_data.temperature -= min_temperature + max_temperature;
  sensor_data.humidity -= min_humidity + max_humidity;
  sensor_data.pressure -= min_pressure + max_pressure;
  sensor_data.co2_concentration -= min_co2_concentration + max_co2_concentration;
  sensor_data.temperature /= SENSOR_N_DATA_FOR_AVERAGE - 2;
  sensor_data.humidity /= SENSOR_N_DATA_FOR_AVERAGE - 2;
  sensor_data.pressure /= SENSOR_N_DATA_FOR_AVERAGE - 2;
  sensor_data.co2_concentration /= SENSOR_N_DATA_FOR_AVERAGE - 2;

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






