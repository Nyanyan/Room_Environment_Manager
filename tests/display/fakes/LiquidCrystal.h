#pragma once
#include <cassert>
#include <string>

class LiquidCrystal {
 public:
  std::string rows[4];
  int x = 0, y = 0, writes = 0, clears = 0;
  LiquidCrystal(int, int, int, int, int, int) {}
  void begin(int columns, int lines) {
    assert(columns == 20 && lines == 4);
    clear();
  }
  void clear() {
    ++clears;
    for (auto &row : rows) row.assign(20, ' ');
    x = y = 0;
  }
  void setCursor(int column, int row) {
    assert(fake_critical_depth == 0);
    assert(column >= 0 && column < 20 && row >= 0 && row < 4);
    x = column; y = row;
  }
  void write(uint8_t value) {
    assert(fake_critical_depth == 0);
    assert(x < 20 && value >= ' ' && value <= '~');
    rows[y][x++] = (char)value;
    ++writes;
  }
  void print(const char *text) { while (*text) write((uint8_t)*text++); }
};
