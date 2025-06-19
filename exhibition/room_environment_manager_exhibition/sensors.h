#ifndef SENSORS_H
#define SENSORS_H

#define SENSOR_N_DATA_FOR_AVERAGE 20

// Temperature & Humidity SHT31
#define SHT31_ADDR 0x45
#define SHT31_SOFT_RESET_MSB 0x30
#define SHT31_SOFT_RESET_LSB 0xA2
#define SHT31_CLEAR_STATUS_REGISTER_MSB 0x30
#define SHT31_CLEAR_STATUS_REGISTER_LSB 0x41
#define SHT31_SINGLE_SHOT_HIGH_MSB 0x24
#define SHT31_SINGLE_SHOT_HIGH_LSB 0x00

// CO2 MH-Z19C
#define MHZ19C_RX_PIN 16
#define MHZ19C_TX_PIN 17

struct Sensor_data{
  float temperature;
  float humidity;
  float pressure;
  float co2_concentration;
};

#endif