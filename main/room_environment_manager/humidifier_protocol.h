#ifndef HUMIDIFIER_PROTOCOL_H
#define HUMIDIFIER_PROTOCOL_H

#include <stdint.h>
#include <string.h>

// Keep identical to main/humidifier/humidifier_protocol.h. Each sketch is standalone.
enum HmState : uint8_t { HM_OFF, HM_WEAK, HM_STRONG, HM_UNKNOWN };
enum HmPacketType : uint8_t { HM_QUERY = 1, HM_SET, HM_STATUS };
enum HmResult : uint8_t { HM_IDLE, HM_BUSY, HM_OK, HM_FAILED, HM_EMPTY };

struct __attribute__((packed)) HmPacket {
  uint8_t magic[4];
  uint32_t sequence;
  uint8_t type;
  uint8_t state;
  uint8_t needs_water;
  uint8_t result;
};

inline HmPacket hm_packet(uint8_t type, uint32_t sequence) {
  HmPacket packet = {{'H', 'M', '0', '1'}, sequence, type, HM_UNKNOWN, 0, HM_IDLE};
  return packet;
}

inline bool hm_packet_valid(const HmPacket &packet) {
  return memcmp(packet.magic, "HM01", 4) == 0 &&
         packet.type >= HM_QUERY && packet.type <= HM_STATUS &&
         packet.state <= HM_UNKNOWN && packet.needs_water <= 1 && packet.result <= HM_EMPTY;
}

static_assert(sizeof(HmPacket) == 12, "Unexpected humidifier packet layout");
#endif
