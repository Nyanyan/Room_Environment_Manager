#ifndef HUMIDIFIER_H
#define HUMIDIFIER_H

#include "command.h"
#include "sensors.h"
#include "time_manager.h"
#include "humidifier_pid.h"

// Optional token.h setting: #define HUMIDIFIER_MAC_ADDR {0x..., ...} (child STA MAC).
// Without it, the feature is disabled and sends no packets.
bool humidifier_configured();
bool humidifier_connected();
void humidifier_exchange_begin();
void humidifier_exchange_end();
void humidifier_receive(const uint8_t *mac, const uint8_t *data, int length);
void humidifier_update(Settings &settings, const Sensor_data &sensors, Time_info &time);
void command_check_hm(Command command, Time_info &time, Settings &settings, const Sensor_data &sensors);
String humidifier_summary(const Settings &settings);

#endif
