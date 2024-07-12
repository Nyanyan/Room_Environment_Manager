#include <Wire.h>
#include <MHZ19_uart.h> // https://github.com/nara256/mhz19_uart
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include "sensors.h"

// CO2 sensor
MHZ19_uart mhz19;



void init_SHT31(){
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
  display_print(0, 1, "[I] SHT31 TH");
}



void init_LPS25HB(){
  Wire.beginTransmission(LPS25HB_ADDR);
  Wire.write(0x20);
  Wire.write(0x90);
  Wire.endTransmission();
  Serial.println("Pressure Sensor Initialized");
  display_print(0, 2, "[I] LPS25HB PR");
}



void init_MHZ19C(){
  mhz19.begin(MHZ19C_RX_PIN, MHZ19C_TX_PIN);
  mhz19.setAutoCalibration(true);
  delay(10000);
  Serial.println("CO2 Sensor Initialized");
  display_print(0, 3, "[I] MH-Z19C CO2");
}



void init_sensors(){
  Wire.begin();
  init_SHT31();
  init_LPS25HB();
  init_MHZ19C();
}



// temperature & humidity
void get_data_SHT31(float *temperature, float *humidity) {
  unsigned int buf[6];
  Wire.beginTransmission(SHT31_ADDR);
  Wire.write(SHT31_SINGLE_SHOT_HIGH_MSB);
  Wire.write(SHT31_SINGLE_SHOT_HIGH_LSB);
  Wire.endTransmission();
  delay(300);
  Wire.requestFrom(SHT31_ADDR, 6);
  for (int i = 0; i < 6; ++i){
    buf[i] = Wire.read();
  }
  Wire.endTransmission();
  uint32_t t = (buf[0] << 8) | buf[1];
  *temperature = (float)t * 175 / 65535.0 - 45.0;
  uint32_t h = (buf[3] << 8) | buf[4];
  *humidity = (float)(h) / 65535.0 * 100.0;
}



// Pressure
float get_data_LPS25HB() {
  Wire.beginTransmission(LPS25HB_ADDR);
  Wire.write(0x28 | 0x80);
  Wire.endTransmission();
  delay(300);
  Wire.requestFrom(LPS25HB_ADDR, 3);
  int32_t pressure_int = Wire.read();
  pressure_int |= (int32_t)Wire.read() << 8;
  pressure_int |= (int32_t)Wire.read() << 16;
  return (float)pressure_int / 4096.0;
}



struct Sensor_data get_sensor_data(){
  float temperatures[SENSOR_N_DATA_FOR_AVERAGE], humidities[SENSOR_N_DATA_FOR_AVERAGE], pressures[SENSOR_N_DATA_FOR_AVERAGE], co2_concentrations[SENSOR_N_DATA_FOR_AVERAGE];
  for (int i = 0; i < SENSOR_N_DATA_FOR_AVERAGE; ++i){
    Serial.print("=");
    get_data_SHT31(&temperatures[i], &humidities[i]);
    pressures[i] = get_data_LPS25HB();
    co2_concentrations[i] = mhz19.getCO2PPM();
    delay(400);
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
  sensor_data.temperature /= SENSOR_N_DATA_FOR_AVERAGE;
  sensor_data.humidity /= SENSOR_N_DATA_FOR_AVERAGE;
  sensor_data.pressure /= SENSOR_N_DATA_FOR_AVERAGE;
  sensor_data.co2_concentration /= SENSOR_N_DATA_FOR_AVERAGE;

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









