#pragma once
#include <array>
#include <cassert>
#include <cstring>
struct FakeEEPROM {
  std::array<uint8_t, 1024> bytes{};
  bool begin(size_t size) { assert(size <= bytes.size()); return true; }
  template<class T> void get(size_t address, T &value) {
    assert(address + sizeof(T) <= bytes.size());
    memcpy(&value, bytes.data() + address, sizeof(T));
  }
  template<class T> void put(size_t address, const T &value) {
    assert(address + sizeof(T) <= bytes.size());
    memcpy(bytes.data() + address, &value, sizeof(T));
  }
  bool commit() { return true; }
};
inline FakeEEPROM EEPROM;
