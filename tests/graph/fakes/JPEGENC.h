#pragma once
#include <cstdint>

// Host tests inspect the palette bitmap before encoding. The ESP32 build checks
// integration with the real JPEGENC library.
constexpr int JPEGE_SUCCESS = 0;
constexpr int JPEGE_PIXEL_RGB888 = 0;
constexpr int JPEGE_SUBSAMPLE_444 = 0;
constexpr int JPEGE_Q_BEST = 0;
struct JPEGENCODE { int cx = 8; int cy = 8; };
class JPEGENC {
 public:
  int open(uint8_t *, int) { return -1; }
  int encodeBegin(JPEGENCODE *, int, int, int, int, int) { return -1; }
  int addMCU(JPEGENCODE *, uint8_t *, int) { return -1; }
  int close() { return 0; }
};
