#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>

using String = std::string;
using std::min;
using std::max;
template <typename T> T constrain(T value, T low, T high) {
  return min(max(value, low), high);
}

#include "graph.ino"

struct GuardedImage {
  uint8_t before[64];
  Graph_img image;
  uint8_t after[64];
  GuardedImage() { std::memset(this, 0xa5, sizeof(*this)); }
  void check() const {
    for (uint8_t value : before) assert(value == 0xa5);
    for (uint8_t value : after) assert(value == 0xa5);
    for (uint8_t value : image.jpeg_buf) assert(value == 0xa5);
    assert(image.jpeg_size == 0xa5a5a5a5);
    for (const auto &row : image.graph) {
      for (uint8_t pixel : row) assert(pixel < BMP_N_COLOR_PALETTE);
    }
  }
};

static Time_info report_time() {
  Time_info time{};
  time.year = 2026; time.month = 11; time.day = 3;
  time.hour = 12; time.minute = 30;
  return time;
}

static Graph_data history() {
  Graph_data data;
  data.last_data_update_hour = 12;
  data.last_data_update_minute = 30;
  for (int i = 0; i < GRAPH_DATA_N; ++i) {
    data.temperature[i] = 23.5f + std::sin(i / 25.0f);
    data.humidity[i] = 45.0f + 4.0f * std::sin(i / 25.0f);
  }
  return data;
}

static void test_control_modes() {
  Settings settings;
  AC_status ac;
  auto refs = graph_control_references(settings, ac);
  assert(!graph_valid_value(refs.temperature_target));
  assert(!graph_valid_value(refs.ac_set_temperature));
  assert(!graph_valid_value(refs.humidity_target));

  for (int mode : {AC_STATE_COOL, AC_STATE_DRY, AC_STATE_HEAT}) {
    ac.state = mode; ac.temp = 22;
    refs = graph_control_references(settings, ac);
    assert(refs.ac_set_temperature == 22);
    assert(!graph_valid_value(refs.temperature_target));
  }
  for (int mode : {AC_AUTO_COOL, AC_AUTO_DRY, AC_AUTO_HEAT}) {
    settings.ac_auto_mode = mode; settings.ac_auto_temp = 26.5;
    settings.hm_auto = true; settings.hm_target_humidity = 57.5;
    refs = graph_control_references(settings, ac);
    assert(refs.temperature_target == 26.5f);
    assert(refs.humidity_target == 57.5f);
    assert(refs.ac_set_temperature == 22);
  }
  ac.state = AC_STATE_OFF; // Auto target remains visible while the AC is stopped.
  refs = graph_control_references(settings, ac);
  assert(refs.temperature_target == 26.5f);
  assert(!graph_valid_value(refs.ac_set_temperature));
  settings.ac_auto_temp = 40;
  assert(graph_control_references(settings, ac).temperature_target == AC_TEMP_LIMIT_MAX);
  settings.ac_auto_temp = 0;
  assert(graph_control_references(settings, ac).temperature_target == AC_TEMP_LIMIT_MIN);
  settings.ac_auto_mode = AC_AUTO_OFF; settings.hm_auto = false;
  refs = graph_control_references(settings, ac);
  assert(!graph_valid_value(refs.temperature_target));
  assert(!graph_valid_value(refs.humidity_target));

  settings.ac_auto_mode = AC_AUTO_COOL; settings.ac_auto_temp = NAN;
  settings.hm_auto = true; settings.hm_target_humidity = INFINITY;
  ac.state = AC_STATE_COOL; ac.temp = -1;
  refs = graph_control_references(settings, ac);
  assert(!graph_valid_value(refs.temperature_target));
  assert(!graph_valid_value(refs.humidity_target));
  assert(!graph_valid_value(refs.ac_set_temperature));
}

static void test_temperature_lines() {
  auto data = history();
  auto time = report_time();
  Settings settings;
  settings.ac_auto_mode = AC_AUTO_COOL; settings.ac_auto_temp = 28;
  AC_status ac; ac.state = AC_STATE_COOL; ac.temp = 18;
  auto refs = graph_control_references(settings, ac);
  const float levels[] = {refs.temperature_target, refs.ac_set_temperature};
  int low, high;
  graph_calculate_range(data.temperature, color_temperature, N_COLOR_TEMPERATURE,
                        GRAPH_TEMPERATURE_SCALE_INTERVAL, &low, &high, levels, 2);
  assert(low <= 18 && high >= 28); // Both references lie outside the measured range.
  GuardedImage guarded;
  graph_draw_temperature(data, guarded.image, time, refs);
  const int target_y = GRAPH_SY + std::lround(120.0f * (28 - low) / (high - low));
  const int setting_y = GRAPH_SY + std::lround(120.0f * (18 - low) / (high - low));
  assert(guarded.image.graph[target_y][GRAPH_SX] == PALETTE_PURPLE);
  assert(guarded.image.graph[setting_y][GRAPH_SX + 2] == PALETTE_BLUE);
  guarded.check();

  // Equal target and setting temperatures must retain both reference colors.
  refs.ac_set_temperature = refs.temperature_target;
  graph_draw_temperature(data, guarded.image, time, refs);
  int coincident_rows = 0;
  for (int y = GRAPH_SY; y <= GRAPH_SY + GRAPH_AREA_HEIGHT; ++y) {
    if (guarded.image.graph[y][GRAPH_SX] == PALETTE_PURPLE &&
        guarded.image.graph[y][GRAPH_SX + 2] == PALETTE_BLUE) {
      ++coincident_rows;
    }
  }
  assert(coincident_rows >= 2);
  guarded.check();

  // Reusing the image for a subsequent manual report must erase the target line.
  settings.ac_auto_mode = AC_AUTO_OFF;
  ac.state = AC_STATE_OFF;
  graph_draw_temperature(data, guarded.image, time, graph_control_references(settings, ac));
  for (int y = GRAPH_SY; y <= GRAPH_SY + GRAPH_AREA_HEIGHT; ++y) {
    for (int x = GRAPH_SX; x < GRAPH_SX + GRAPH_AREA_WIDTH; ++x) {
      assert(guarded.image.graph[y][x] != PALETTE_PURPLE);
      assert(guarded.image.graph[y][x] != PALETTE_BLUE);
    }
  }
  guarded.check();
}

static void test_humidity_and_empty_history() {
  auto time = report_time();
  Settings settings; settings.hm_auto = true;
  AC_status ac;
  for (bool empty : {false, true}) {
    auto data = empty ? Graph_data() : history();
    data.humidity[0] = NAN; data.humidity[1] = INFINITY; data.humidity[2] = FLT_MAX;
    for (float target : {0.0f, 55.5f, 100.0f}) {
      settings.hm_target_humidity = target;
      auto refs = graph_control_references(settings, ac);
      int low, high;
      graph_calculate_range(data.humidity, color_humidity, N_COLOR_HUMIDITY,
                            GRAPH_HUMIDITY_SCALE_INTERVAL, &low, &high, &target, 1);
      assert(low <= target && high >= target && low < high);
      GuardedImage guarded;
      graph_draw_humidity(data, guarded.image, time, refs);
      const int y = GRAPH_SY + std::lround(120.0f * (target - low) / (high - low));
      assert(guarded.image.graph[y][GRAPH_SX] == PALETTE_PURPLE);
      assert(guarded.image.graph[y + 1][GRAPH_SX] == PALETTE_PURPLE);
      guarded.check();
    }
  }
  Graph_data empty;
  Graph_references none;
  GuardedImage guarded;
  graph_draw_temperature(empty, guarded.image, time, none); guarded.check();
  graph_draw_humidity(empty, guarded.image, time, none); guarded.check();
  graph_draw_pressure(empty, guarded.image, time); guarded.check();
  graph_draw_co2_concentration(empty, guarded.image, time); guarded.check();
}

static void test_missing_sample_and_day_boundary() {
  Graph_data data;
  data.temperature[GRAPH_DATA_1_DAY - 1] = 0;
  data.temperature[GRAPH_DATA_1_DAY] = 100;
  data.temperature[GRAPH_DATA_1_DAY + 2] = 0; // There is a missing sample in between.
  GuardedImage guarded;
  graph_draw_white(guarded.image);
  graph_plot(guarded.image, data.temperature, 0, 100);
  for (int y = GRAPH_SY; y <= GRAPH_SY + GRAPH_AREA_HEIGHT; ++y) {
    assert(guarded.image.graph[y][GRAPH_SX - 1] == PALETTE_WHITE);
    assert(guarded.image.graph[y][GRAPH_SX + 1] == PALETTE_WHITE);
  }
  guarded.check();
}

static void save_ppm(const Graph_img &image, const std::string &path) {
  std::ofstream file(path, std::ios::binary);
  file << "P6\n" << GRAPH_IMG_WIDTH << ' ' << GRAPH_IMG_HEIGHT << "\n255\n";
  for (int y = GRAPH_IMG_HEIGHT - 1; y >= 0; --y) {
    for (int x = 0; x < GRAPH_IMG_WIDTH; ++x) {
      file.write(reinterpret_cast<const char *>(color_palette[image.graph[y][x]]), 3);
    }
  }
  assert(file.good());
}

int main(int argc, char **argv) {
  test_control_modes();
  test_temperature_lines();
  test_humidity_and_empty_history();
  test_missing_sample_and_day_boundary();
  if (argc == 2) {
    auto data = history(); auto time = report_time();
    Settings settings;
    settings.ac_auto_mode = AC_AUTO_COOL; settings.ac_auto_temp = 26;
    settings.hm_auto = true; settings.hm_target_humidity = 55;
    AC_status ac; ac.state = AC_STATE_COOL; ac.temp = 20;
    auto refs = graph_control_references(settings, ac);
    Graph_img image;
    graph_draw_temperature(data, image, time, refs);
    save_ppm(image, std::string(argv[1]) + "/temperature.ppm");
    graph_draw_humidity(data, image, time, refs);
    save_ppm(image, std::string(argv[1]) + "/humidity.ppm");
  }
  std::cout << "All graph tests passed.\n";
}
