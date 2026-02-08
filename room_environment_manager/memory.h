#ifndef MEMORY_H
#define MEMORY_H

#include "air_conditioner.h"

// Initialize EEPROM for persisting AC status.
void memory_init();

// Save current AC status into EEPROM. Returns true on success.
bool memory_save_ac_status(const AC_status &ac_status);

// Load last saved AC status from EEPROM. Returns true when valid data is restored.
bool memory_load_ac_status(AC_status &ac_status);

#endif