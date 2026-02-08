#include <EEPROM.h>
#include "memory.h"

namespace {

// Keep payload small; ESP32 EEPROM is emulated via flash.
constexpr uint32_t kMagic = 0xAC5EED01; // identifies valid payload
constexpr uint8_t kVersion = 1;
constexpr size_t kEepromSize = 64; // bytes reserved for this module
constexpr int kAcStatusAddr = 0;   // start address for AC status block

struct AcPersisted {
	uint32_t magic;
	uint8_t version;
	int32_t state;
	int32_t temp;
	int32_t hot_count;
	int32_t cold_count;
	uint32_t checksum; // simple additive checksum over preceding bytes
};

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

} // namespace


void memory_init() {
	ensure_initialized();
}


bool memory_save_ac_status(const AC_status &ac_status) {
	ensure_initialized();
	AcPersisted payload = {};
	payload.magic = kMagic;
	payload.version = kVersion;
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

	if (payload.magic != kMagic || payload.version != kVersion) {
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
