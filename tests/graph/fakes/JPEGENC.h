#pragma once
#include <cstdint>
#include <vector>

// Capture the actual BGR pixels passed to the encoder, without compressing them.
inline std::vector<uint8_t> jpeg_test_mcus;
constexpr int JPEGE_SUCCESS = 0;
constexpr int JPEGE_PIXEL_RGB888 = 0;
constexpr int JPEGE_SUBSAMPLE_444 = 0;
constexpr int JPEGE_Q_BEST = 0;
struct JPEGENCODE { int cx = 8; int cy = 8; };
class JPEGENC {
 public:
  int open(uint8_t *, int) { jpeg_test_mcus.clear(); return JPEGE_SUCCESS; }
  int encodeBegin(JPEGENCODE *, int, int, int, int, int) { return JPEGE_SUCCESS; }
  int addMCU(JPEGENCODE *jpe, uint8_t *pixels, int stride) {
    jpeg_test_mcus.insert(jpeg_test_mcus.end(), pixels, pixels + stride * jpe->cy);
    return JPEGE_SUCCESS;
  }
  int close() { return 128; }
};
