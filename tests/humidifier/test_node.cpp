#include <cassert>
#include <iostream>
#include "humidifier.ino"

uint8_t physical_state = HM_OFF;
bool physical_water = false;
bool actuator_moves = true;
bool pressed = false;
int pulse_count = 0;

void update_inputs() {
  fake_pins[LED_WORKING_WEAK] = (physical_state == HM_WEAK || physical_state == HM_UNKNOWN) ? LOW : HIGH;
  fake_pins[LED_WORKING_STRONG] = (physical_state == HM_STRONG || physical_state == HM_UNKNOWN) ? LOW : HIGH;
  fake_pins[LED_NEED_WATER] = physical_water ? LOW : HIGH;
}
void run(uint32_t ms) {
  for (uint32_t i = 0; i < ms; ++i) { update_inputs(); loop(); }
}
void send(uint8_t type, uint8_t state, uint32_t sequence) {
  HmPacket request = hm_packet(type, sequence);
  request.state = state;
  OnHumidifierDataRecv(parent_mac, reinterpret_cast<const uint8_t *>(&request), sizeof(request));
}
int main() {
  fake_pin_write = [](int pin, int level) {
    if (pin != BUTTON) return;
    if (level == HUMIDIFIER_BUTTON_ACTIVE_LEVEL) {
      assert(!pressed); pressed = true; ++pulse_count;
    } else if (pressed) {
      pressed = false;
      if (actuator_moves) physical_state = (physical_state + 1) % 3;
    }
  };
  setup(); run(100);
  assert(!pressed && pulse_count == 0 && radio_ready);
  uint32_t sequence = 0;
  for (uint8_t from = HM_OFF; from <= HM_STRONG; ++from) {
    for (uint8_t to = HM_OFF; to <= HM_STRONG; ++to) {
      physical_state = from; run(100); pulse_count = 0;
      send(HM_SET, to, ++sequence); run(1500);
      assert(physical_state == to && command_result == HM_OK && !pressed);
      assert(pulse_count == (to + 3 - from) % 3);
      int before = pulse_count;
      send(HM_SET, to, sequence); run(1500);
      assert(pulse_count == before); // duplicate sequence never toggles again
    }
  }
  physical_state = HM_OFF; physical_water = true; run(100); pulse_count = 0;
  send(HM_SET, HM_STRONG, ++sequence); run(1500);
  assert(command_result == HM_EMPTY && pulse_count == 0 && !pressed);
  send(HM_QUERY, HM_UNKNOWN, ++sequence); run(10);
  HmPacket reply; memcpy(&reply, fake_packets.back().data(), sizeof(reply));
  assert(reply.needs_water && reply.state == HM_OFF);
  physical_water = false; physical_state = HM_UNKNOWN; run(100);
  send(HM_SET, HM_WEAK, ++sequence); run(1500);
  assert(command_result == HM_FAILED && pulse_count == 0);
  physical_state = HM_OFF; actuator_moves = false; run(100);
  send(HM_SET, HM_WEAK, ++sequence); run(1500);
  assert(command_result == HM_FAILED && pulse_count == 3 && !pressed && !controlling);
  actuator_moves = true; pulse_count = 0;
  HmPacket request = hm_packet(HM_SET, ++sequence); request.state = HM_STRONG;
  uint8_t stranger[6]; memcpy(stranger, parent_mac, 6); stranger[0] ^= 1;
  OnHumidifierDataRecv(stranger, reinterpret_cast<uint8_t *>(&request), sizeof(request));
  OnHumidifierDataRecv(parent_mac, reinterpret_cast<uint8_t *>(&request), 1);
  request.magic[0] = '!';
  OnHumidifierDataRecv(parent_mac, reinterpret_cast<uint8_t *>(&request), sizeof(request));
  run(1500); assert(pulse_count == 0);
  send(HM_SET, HM_STRONG, ++sequence); run(1500); assert(physical_state == HM_STRONG);
  fake_now += HM_LEASE_MS; run(1500);
  assert(physical_state == HM_OFF && !remote_lease && !pressed);
  std::cout << "Node: all 9 transitions, duplicate/invalid packets, water, stuck LEDs and parent outage passed\n";
}
