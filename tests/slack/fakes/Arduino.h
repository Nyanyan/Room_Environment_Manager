#pragma once
#include <algorithm>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

class String : public std::string {
  // Arduino String has no public iterator typedef; avoid selecting both of
  // ArduinoJson's Arduino-String and STL-container reader specializations.
  using const_iterator = std::string::const_iterator;
 public:
  using std::string::string;
  using std::string::operator=;
  String(const std::string &value) : std::string(value) {}
  String(int value) : std::string(std::to_string(value)) {}
  String(double value, int decimals) {
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(decimals) << value;
    assign(stream.str());
  }
  String &operator=(const char *value) { assign(value ? value : ""); return *this; }
  bool concat(const char *value, unsigned int size) { append(value, size); return true; }
  bool concat(const char *value) { append(value); return true; }
  void trim() {
    const auto first = find_first_not_of(" \r\n\t");
    if (first == npos) { clear(); return; }
    *this = substr(first, find_last_not_of(" \r\n\t") - first + 1);
  }
};
inline uint32_t fake_now = 0;
inline uint32_t millis() { return fake_now; }
inline void delay(uint32_t ms) { fake_now += ms; }
inline uint32_t esp_random() { return 0x12345678; }
struct FakeSerial {
  std::vector<std::string> lines;
  void println(const String &text) { lines.push_back(text); }
  void printf(const char *format, ...) {
    char buffer[512];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    lines.emplace_back(buffer);
  }
};
inline FakeSerial Serial;
