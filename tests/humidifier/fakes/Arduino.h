#pragma once
#include <algorithm>
#include <cmath>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

class String {
  std::string value;
public:
  String() = default;
  String(const char *text) : value(text) {}
  String(const std::string &text) : value(text) {}
  String(int number) : value(std::to_string(number)) {}
  String(unsigned int number) : value(std::to_string(number)) {}
  String(unsigned long number) : value(std::to_string(number)) {}
  String(double number, int decimals = 2) {
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(decimals) << number;
    value = stream.str();
  }
  size_t length() const { return value.size(); }
  const char *c_str() const { return value.c_str(); }
  char charAt(size_t index) const { return index < value.size() ? value[index] : 0; }
  int indexOf(char c, size_t from = 0) const {
    const auto index = value.find(c, from);
    return index == std::string::npos ? -1 : static_cast<int>(index);
  }
  String substring(size_t from, size_t to = std::string::npos) const {
    return from >= value.size() ? String() : String(value.substr(from, to - from));
  }
  bool startsWith(const String &prefix) const { return value.rfind(prefix.value, 0) == 0; }
  void remove(size_t from, size_t count) { value.erase(from, count); }
  void toLowerCase() {
    for (char &c : value) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  }
  void trim() {
    const auto first = value.find_first_not_of(" \r\n\t");
    value = first == std::string::npos ? "" : value.substr(first, value.find_last_not_of(" \r\n\t") - first + 1);
  }
  long toInt() const { return std::strtol(value.c_str(), nullptr, 10); }
  double toDouble() const { return std::strtod(value.c_str(), nullptr); }
  String &operator+=(const String &other) { value += other.value; return *this; }
  friend String operator+(String lhs, const String &rhs) { return lhs += rhs; }
  friend bool operator==(const String &lhs, const String &rhs) { return lhs.value == rhs.value; }
  friend bool operator!=(const String &lhs, const String &rhs) { return !(lhs == rhs); }
};

constexpr int LOW = 0, HIGH = 1, INPUT = 0, OUTPUT = 1;
inline uint32_t fake_now = 1;
inline int fake_pins[40] = {};
inline std::function<void(int, int)> fake_pin_write;
inline uint32_t millis() { return fake_now; }
inline void delay(uint32_t ms) { fake_now += ms; }
inline int digitalRead(int pin) { return fake_pins[pin]; }
inline void digitalWrite(int pin, int level) {
  fake_pins[pin] = level;
  if (fake_pin_write) fake_pin_write(pin, level);
}
inline void pinMode(int, int) {}
using portMUX_TYPE = int;
#define portMUX_INITIALIZER_UNLOCKED 0
#define portENTER_CRITICAL(x) ((void)(x))
#define portEXIT_CRITICAL(x) ((void)(x))
inline uint32_t esp_random() { static uint32_t id = 100; return ++id; }
struct FakeSerial {
  void begin(int) {}
  template<class T> void print(T) {}
  template<class T> void println(T) {}
};
inline FakeSerial Serial;
