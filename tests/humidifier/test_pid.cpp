#include <cassert>
#include <cfloat>
#include <iostream>
#include "humidifier_pid.h"

int main() {
  HmPid pid;
  assert(!hm_valid_humidity(NAN) && !hm_valid_humidity(INFINITY));
  assert(!hm_valid_humidity(FLT_MAX) && !hm_valid_humidity(-1) && !hm_valid_humidity(101));
  assert(hm_valid_humidity(0) && hm_valid_humidity(100));
  assert(pid.update(50, 30, 0));
  assert(pid.output == 100 && pid.state(HM_OFF) == HM_STRONG);
  for (uint32_t now = 30000; now <= 3600000; now += 30000) pid.update(50, 30, now);
  assert(pid.integral == 0); // one hour of saturation must not wind up
  pid.update(50, 55, 3630000);
  assert(pid.output == 0 && pid.state(HM_STRONG) == HM_OFF);

  pid.reset();
  pid.update(50, 47, 0);
  assert(pid.state(HM_OFF) == HM_WEAK);
  const double initial = pid.output;
  pid.update(50, 47, 30000);
  assert(pid.output > initial); // integral compensates steady humidity loss
  HmPid steady = pid;
  pid.update(50, 48, 60000);
  steady.update(50, 47, 60000);
  assert(pid.filtered_rate > 0 && pid.output < steady.output); // rising humidity reduces output
  assert(pid.filtered_rate < 1.0 / 30); // derivative is filtered
  assert(!pid.update(50, 48, 60001));
  assert(!pid.update(50, NAN, 90000) && !pid.initialized);
  pid.update(100, 0, 0);
  assert(pid.output == 100);
  pid.update(0, 0, 30000);
  assert(pid.output == 0);
  pid.reset();
  pid.update(50, 48, UINT32_MAX - 15000);
  assert(pid.update(50, 48, 15000)); // millis rollover
  pid.update(50, 48, 500000);
  assert(pid.integral == 0 && pid.filtered_rate == 0); // long sample gap
  pid.output = 20;
  assert(pid.state(HM_OFF) == HM_OFF && pid.state(HM_WEAK) == HM_WEAK);
  pid.output = 60;
  assert(pid.state(HM_STRONG) == HM_STRONG && pid.state(HM_WEAK) == HM_WEAK);
  std::cout << "PID: limits, anti-windup, I/D response, hysteresis, invalid samples and rollover passed\n";
}
