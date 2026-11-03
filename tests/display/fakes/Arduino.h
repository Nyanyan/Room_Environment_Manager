#pragma once
#include "../../humidifier/fakes/Arduino.h"
#include <cassert>
#include <ctime>

inline int fake_critical_depth = 0;
#undef portENTER_CRITICAL
#undef portEXIT_CRITICAL
#define portENTER_CRITICAL(mux) ((void)(mux), ++fake_critical_depth)
#define portEXIT_CRITICAL(mux) ((void)(mux), --fake_critical_depth)

inline bool fake_clock_valid = true;
inline int fake_clock_base = 12 * 3600 + 34 * 60 + 56;
inline uint32_t fake_clock_started_ms = 1;
inline bool getLocalTime(struct tm *now, uint32_t timeout) {
  assert(timeout == 0 && fake_critical_depth == 0);
  if (!fake_clock_valid) return false;
  const int seconds = (fake_clock_base + (fake_now - fake_clock_started_ms) / 1000) % 86400;
  *now = {};
  now->tm_year = 126;
  now->tm_hour = seconds / 3600;
  now->tm_min = seconds / 60 % 60;
  now->tm_sec = seconds % 60;
  return true;
}
