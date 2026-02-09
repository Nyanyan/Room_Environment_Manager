#ifndef ADDITIONAL_SENSORS_H
#define ADDITIONAL_SENSORS_H

#include "sensors.h"
#include "token.h"

// Trigger an ESP-NOW request to all additional sensors and update cached data.
void additional_sensors_request();

// Check if the idx-th additional sensor responded in the latest request.
bool additional_sensor_received(int idx);

// Get the last received data for the idx-th additional sensor.
SensorReading additional_sensor_data_get(int idx);

#endif
