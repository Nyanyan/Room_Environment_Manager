#include <EEPROM.h>
#include "memory.h"

namespace {

// Keep payload small; ESP32 EEPROM is emulated via flash.
struct AcPersisted {
	uint32_t magic;
	uint8_t version;
	int32_t state;
	int32_t temp;
	int32_t hot_count;
	int32_t cold_count;
	uint32_t checksum; // simple additive checksum over preceding bytes
};

struct SettingsPersisted {
	uint32_t magic;
	uint8_t version;
	uint8_t alert_when_hot; // bool stored as byte
	int32_t ac_auto_mode;
	float ac_auto_temp;
	uint8_t reserved[7]; // keep alignment / future use
	uint32_t checksum;
};

constexpr uint32_t kMagicAc = 0xAC5EED01; // identifies valid AC payload
constexpr uint8_t kVersionAc = 1;
constexpr uint32_t kMagicSettings = 0x57AABBCC; // identifies valid Settings payload
constexpr uint8_t kVersionSettings = 1;
constexpr uint32_t kReservationMagic = 0xAC5EED02; // identifies reservation payload
constexpr uint8_t kReservationVersion = 1;
constexpr size_t kEepromSize = 1024; // bytes reserved for this module
constexpr int kAcStatusAddr = 0;   // start address for AC status block
constexpr int kSettingsAddr = kAcStatusAddr + sizeof(AcPersisted);
constexpr int kReservationAddr = kSettingsAddr + sizeof(SettingsPersisted);

bool g_initialized = false;

uint32_t compute_checksum(const AcPersisted &data) {
	uint32_t sum = 0;
	const uint8_t *raw = reinterpret_cast<const uint8_t *>(&data);
	const size_t checksum_offset = sizeof(AcPersisted) - sizeof(uint32_t);
	for (size_t i = 0; i < checksum_offset; ++i) {
		sum += raw[i];
	}
	return sum;
}

uint32_t compute_checksum(const SettingsPersisted &data) {
	uint32_t sum = 0;
	const uint8_t *raw = reinterpret_cast<const uint8_t *>(&data);
	const size_t checksum_offset = sizeof(SettingsPersisted) - sizeof(uint32_t);
	for (size_t i = 0; i < checksum_offset; ++i) {
		sum += raw[i];
	}
	return sum;
}

void ensure_initialized() {
	if (g_initialized) {
		return;
	}
	EEPROM.begin(kEepromSize);
	g_initialized = true;
}

bool is_state_valid(int32_t state) {
	return state == AC_STATE_OFF || state == AC_STATE_COOL || state == AC_STATE_DRY || state == AC_STATE_HEAT;
}

struct ReservationBlock {
	uint32_t magic;
	uint8_t version;
	uint8_t padding[3]; // keep alignment predictable
	uint32_t next_id;
	CommandReservation slots[RESERVATION_MAX];
	uint32_t checksum;
};

uint32_t compute_checksum(const ReservationBlock &block) {
	uint32_t sum = 0;
	const uint8_t *raw = reinterpret_cast<const uint8_t *>(&block);
	const size_t checksum_offset = sizeof(ReservationBlock) - sizeof(uint32_t);
	for (size_t i = 0; i < checksum_offset; ++i) {
		sum += raw[i];
	}
	return sum;
}

ReservationBlock empty_reservation_block() {
	ReservationBlock block = {};
	block.magic = kReservationMagic;
	block.version = kReservationVersion;
	block.next_id = 1;
	for (size_t i = 0; i < RESERVATION_MAX; ++i) {
		block.slots[i] = CommandReservation();
	}
	block.checksum = compute_checksum(block);
	return block;
}

ReservationBlock load_reservation_block() {
	ensure_initialized();
	ReservationBlock block = {};
	EEPROM.get(kReservationAddr, block);
	bool valid = block.magic == kReservationMagic && block.version == kReservationVersion && block.checksum == compute_checksum(block);
	if (!valid) {
		block = empty_reservation_block();
		EEPROM.put(kReservationAddr, block);
		EEPROM.commit();
	}
	return block;
}

bool save_reservation_block(ReservationBlock &block) {
	block.checksum = compute_checksum(block);
	EEPROM.put(kReservationAddr, block);
	return EEPROM.commit();
}

bool is_due_or_past(const CommandReservation &reservation, const Time_info &now) {
	if (reservation.year < now.year) return true;
	if (reservation.year > now.year) return false;
	if (reservation.month < now.month) return true;
	if (reservation.month > now.month) return false;
	if (reservation.day < now.day) return true;
	if (reservation.day > now.day) return false;
	if (reservation.hour < now.hour) return true;
	if (reservation.hour > now.hour) return false;
	return reservation.minute <= now.minute;
}

bool is_earlier(const CommandReservation &lhs, const CommandReservation &rhs) {
	if (lhs.year != rhs.year) return lhs.year < rhs.year;
	if (lhs.month != rhs.month) return lhs.month < rhs.month;
	if (lhs.day != rhs.day) return lhs.day < rhs.day;
	if (lhs.hour != rhs.hour) return lhs.hour < rhs.hour;
	return lhs.minute < rhs.minute;
}

} // namespace


void memory_init() {
	ensure_initialized();
}


bool memory_save_ac_status(const AC_status &ac_status) {
	ensure_initialized();
	AcPersisted payload = {};
	payload.magic = kMagicAc;
	payload.version = kVersionAc;
	payload.state = ac_status.state;
	payload.temp = ac_status.temp;
	payload.hot_count = ac_status.hot_count;
	payload.cold_count = ac_status.cold_count;
	payload.checksum = compute_checksum(payload);

	EEPROM.put(kAcStatusAddr, payload);
	return EEPROM.commit();
}


bool memory_load_ac_status(AC_status &ac_status) {
	ensure_initialized();
	AcPersisted payload = {};
	EEPROM.get(kAcStatusAddr, payload);

	if (payload.magic != kMagicAc || payload.version != kVersionAc) {
		ac_status = AC_status();
		return false;
	}

	if (payload.checksum != compute_checksum(payload)) {
		ac_status = AC_status();
		return false;
	}

	ac_status.state = is_state_valid(payload.state) ? payload.state : AC_STATE_OFF;

	if (payload.temp < AC_TEMP_LIMIT_MIN || payload.temp > AC_TEMP_LIMIT_MAX) {
		ac_status.temp = -1; // unknown/invalid, force manual set
	} else {
		ac_status.temp = payload.temp;
	}

	ac_status.hot_count = payload.hot_count;
	ac_status.cold_count = payload.cold_count;
	return true;
}


bool memory_save_settings(const Settings &settings) {
	ensure_initialized();
	SettingsPersisted payload = {};
	payload.magic = kMagicSettings;
	payload.version = kVersionSettings;
	payload.alert_when_hot = settings.alert_when_hot ? 1 : 0;
	payload.ac_auto_mode = settings.ac_auto_mode;
	payload.ac_auto_temp = static_cast<float>(settings.ac_auto_temp);
	payload.checksum = compute_checksum(payload);

	EEPROM.put(kSettingsAddr, payload);
	return EEPROM.commit();
}


bool memory_load_settings(Settings &settings) {
	ensure_initialized();
	SettingsPersisted payload = {};
	EEPROM.get(kSettingsAddr, payload);

	if (payload.magic != kMagicSettings || payload.version != kVersionSettings) {
		settings = Settings();
		return false;
	}

	if (payload.checksum != compute_checksum(payload)) {
		settings = Settings();
		return false;
	}

	settings.alert_when_hot = payload.alert_when_hot != 0;
	if (payload.ac_auto_mode < AC_AUTO_OFF || payload.ac_auto_mode > AC_AUTO_HEAT) {
		settings.ac_auto_mode = AC_AUTO_OFF;
	} else {
		settings.ac_auto_mode = payload.ac_auto_mode;
	}
	settings.ac_auto_temp = payload.ac_auto_temp;
	return true;
}



bool reservation_add(const CommandReservation &reservation, uint32_t &assigned_id) {
	ReservationBlock block = load_reservation_block();
	int free_index = -1;
	for (size_t i = 0; i < RESERVATION_MAX; ++i) {
		if (!block.slots[i].active) {
			free_index = static_cast<int>(i);
			break;
		}
	}
	if (free_index < 0) {
		return false;
	}
	CommandReservation to_store = reservation;
	to_store.id = block.next_id++;
	to_store.active = 1;
	block.slots[free_index] = to_store;
	assigned_id = to_store.id;
	return save_reservation_block(block);
}


bool reservation_delete(uint32_t id) {
	ReservationBlock block = load_reservation_block();
	bool found = false;
	for (size_t i = 0; i < RESERVATION_MAX; ++i) {
		if (block.slots[i].active && block.slots[i].id == id) {
			block.slots[i] = CommandReservation();
			found = true;
			break;
		}
	}
	if (!found) {
		return false;
	}
	return save_reservation_block(block);
}


bool reservation_list(CommandReservation *reservations, size_t max_count, size_t &count_out) {
	ReservationBlock block = load_reservation_block();
	count_out = 0;
	for (size_t i = 0; i < RESERVATION_MAX; ++i) {
		if (block.slots[i].active) {
			if (count_out < max_count) {
				reservations[count_out] = block.slots[i];
			}
			++count_out;
		}
	}
	return true;
}


bool reservation_pop_due(const Time_info &now, CommandReservation &reservation_out) {
	ReservationBlock block = load_reservation_block();
	int candidate_index = -1;
	for (size_t i = 0; i < RESERVATION_MAX; ++i) {
		if (!block.slots[i].active) {
			continue;
		}
		if (!is_due_or_past(block.slots[i], now)) {
			continue;
		}
		if (candidate_index == -1 || is_earlier(block.slots[i], block.slots[candidate_index])) {
			candidate_index = static_cast<int>(i);
		}
	}
	if (candidate_index == -1) {
		return false;
	}
	reservation_out = block.slots[candidate_index];
	block.slots[candidate_index] = CommandReservation();
	save_reservation_block(block);
	return true;
}
