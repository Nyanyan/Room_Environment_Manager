#ifndef SENSORS_H
#define SENSORS_H

#include <float.h>
#include "token.h"

#define SENSOR_N_DATA_FOR_AVERAGE 25

struct SensorReading{
  float temperature;
  float humidity;
  float pressure;
  float co2_concentration;

  SensorReading() : temperature(FLT_MAX), humidity(FLT_MAX), pressure(FLT_MAX), co2_concentration(FLT_MAX) {}
};

struct Sensor_data{
  SensorReading parent;                           // values measured by the single sensor
};

Sensor_data get_sensor_data();

#endif