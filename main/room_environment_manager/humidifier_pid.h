#ifndef HUMIDIFIER_PID_H
#define HUMIDIFIER_PID_H

#include <math.h>
#include <stdint.h>
#include "humidifier_protocol.h"

// Starting values, to be tuned against the room's measured step response.
// Output: 0..100%; error: percentage points RH; time: seconds.
constexpr double HM_PID_KP = 12.0;
constexpr double HM_PID_KI = 0.015;
constexpr double HM_PID_KD = 300.0;
constexpr double HM_PID_D_FILTER_SECONDS = 120.0;
constexpr uint32_t HM_PID_SAMPLE_MS = 30000;
constexpr uint32_t HM_SWITCH_INTERVAL_MS = 120000;

inline bool hm_valid_humidity(double value) {
  return isfinite(value) && value >= 0.0 && value <= 100.0;
}

struct HmPid {
  bool initialized = false;
  uint32_t last_ms = 0;
  double previous_humidity = 0;
  double integral = 0;
  double filtered_rate = 0;
  double output = 0;

  void reset() { *this = HmPid(); }

  bool update(double target, double humidity, uint32_t now) {
    if (!hm_valid_humidity(target) || !hm_valid_humidity(humidity)) {
      reset();
      return false;
    }
    double dt = 0;
    if (initialized) {
      uint32_t elapsed = now - last_ms; // also handles millis() rollover
      if (elapsed < HM_PID_SAMPLE_MS) return false;
      if (elapsed <= 300000) dt = elapsed / 1000.0;
      else reset(); // do not integrate a long communication/sampling gap
    }
    const double error = target - humidity;
    if (dt > 0) {
      const double rate = (humidity - previous_humidity) / dt;
      filtered_rate += dt / (HM_PID_D_FILTER_SECONDS + dt) * (rate - filtered_rate);
      const double raw = HM_PID_KP * error + integral - HM_PID_KD * filtered_rate;
      // Conditional integration: let the integral unwind, but not wind up at saturation.
      if ((raw < 100.0 || error < 0) && (raw > 0.0 || error > 0)) {
        integral = fmax(0.0, fmin(100.0, integral + HM_PID_KI * error * dt));
      }
    }
    output = fmax(0.0, fmin(100.0, HM_PID_KP * error + integral - HM_PID_KD * filtered_rate));
    if (target == 0.0) { output = 0; integral = 0; }
    previous_humidity = humidity;
    last_ms = now;
    initialized = true;
    return true;
  }

  uint8_t state(uint8_t current) const {
    // Schmitt thresholds prevent jitter between OFF/WEAK/STRONG.
    if (current == HM_STRONG && output >= 55.0) return HM_STRONG;
    if (output >= 70.0) return HM_STRONG;
    if (current != HM_OFF && current != HM_UNKNOWN && output >= 10.0) return HM_WEAK;
    return output >= 25.0 ? HM_WEAK : HM_OFF;
  }
};
#endif
