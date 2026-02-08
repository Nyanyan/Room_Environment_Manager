#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>
#include "air_conditioner.h"
#include "time_manager.h"

// Reservation capacity and limits
const size_t RESERVATION_MAX = 4;
const size_t RESERVATION_COMMAND_MAX_LEN = 80;

struct CommandReservation {
	uint32_t id;
	uint16_t year;
	uint8_t month;
	uint8_t day;
	uint8_t hour;
	uint8_t minute;
	char command[RESERVATION_COMMAND_MAX_LEN];
	uint8_t active;

	CommandReservation() : id(0), year(0), month(0), day(0), hour(0), minute(0), active(0) {
		command[0] = '\0';
	}
};

// Initialize EEPROM for persisting AC status.
void memory_init();

// Save current AC status into EEPROM. Returns true on success.
bool memory_save_ac_status(const AC_status &ac_status);

// Load last saved AC status from EEPROM. Returns true when valid data is restored.
bool memory_load_ac_status(AC_status &ac_status);

// Add a new reservation to EEPROM. Returns false when storage is full.
bool reservation_add(const CommandReservation &reservation, uint32_t &assigned_id);

// Delete reservation with matching id. Returns true when found and deleted.
bool reservation_delete(uint32_t id);

// List all active reservations up to max_count, writing them into reservations. count_out is set to the number returned.
bool reservation_list(CommandReservation *reservations, size_t max_count, size_t &count_out);

// Pop the oldest reservation that is due at or before now. Returns true when one is found and removed.
bool reservation_pop_due(const Time_info &now, CommandReservation &reservation_out);

#endif