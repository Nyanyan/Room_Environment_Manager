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

static void fill_controls(Graph_data &data, const Graph_references &refs) {
  std::fill_n(data.temperature_target, GRAPH_DATA_1_DAY, refs.temperature_target);
  std::fill_n(data.ac_set_temperature, GRAPH_DATA_1_DAY, refs.ac_set_temperature);
  std::fill_n(data.humidity_target, GRAPH_DATA_1_DAY, refs.humidity_target);
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
  fill_controls(data, refs);
  int low, high;
  graph_calculate_range(data.temperature, color_temperature, N_COLOR_TEMPERATURE,
                        GRAPH_TEMPERATURE_SCALE_INTERVAL, &low, &high, data.temperature_target, data.ac_set_temperature);
  assert(low <= 18 && high >= 28); // Both references lie outside the measured range.
  GuardedImage guarded;
  graph_draw_temperature(data, guarded.image, time);
  const int target_y = GRAPH_SY + std::lround(120.0f * (28 - low) / (high - low));
  const int setting_y = GRAPH_SY + std::lround(120.0f * (18 - low) / (high - low));
  assert(guarded.image.graph[target_y][GRAPH_SX] == PALETTE_PURPLE);
  assert(guarded.image.graph[setting_y][GRAPH_SX + 2] == PALETTE_BLUE);
  guarded.check();

  // Equal target and setting temperatures must retain both reference colors.
  refs.ac_set_temperature = refs.temperature_target;
  fill_controls(data, refs);
  graph_draw_temperature(data, guarded.image, time);
  int coincident_rows = 0;
  for (int y = GRAPH_SY; y <= GRAPH_SY + GRAPH_AREA_HEIGHT; ++y) {
    if (guarded.image.graph[y][GRAPH_SX + 1] == PALETTE_PURPLE &&
        guarded.image.graph[y][GRAPH_SX + 2] == PALETTE_BLUE) {
      ++coincident_rows;
    }
  }
  assert(coincident_rows >= 1);
  guarded.check();

  // A history with no active modes must not draw reference lines.
  settings.ac_auto_mode = AC_AUTO_OFF;
  ac.state = AC_STATE_OFF;
  fill_controls(data, graph_control_references(settings, ac));
  graph_draw_temperature(data, guarded.image, time);
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
      fill_controls(data, refs);
      int low, high;
      graph_calculate_range(data.humidity, color_humidity, N_COLOR_HUMIDITY,
                            GRAPH_HUMIDITY_SCALE_INTERVAL, &low, &high, data.humidity_target, nullptr);
      assert(low <= target && high >= target && low < high);
      GuardedImage guarded;
      graph_draw_humidity(data, guarded.image, time);
      const int y = GRAPH_SY + std::lround(120.0f * (target - low) / (high - low));
      assert(guarded.image.graph[y][GRAPH_SX] == PALETTE_PURPLE);
      assert(guarded.image.graph[y + 1][GRAPH_SX] == PALETTE_PURPLE);
      guarded.check();
    }
  }
  Graph_data empty;
  GuardedImage guarded;
  graph_draw_temperature(empty, guarded.image, time); guarded.check();
  graph_draw_humidity(empty, guarded.image, time); guarded.check();
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

static void test_setting_history() {
  Graph_data data;
  Sensor_data sensor;
  sensor.representative.temperature = 23;
  sensor.representative.humidity = 45;
  Settings settings;
  settings.ac_auto_mode = AC_AUTO_COOL; settings.ac_auto_temp = 26;
  settings.hm_auto = true; settings.hm_target_humidity = 55;
  AC_status ac; ac.state = AC_STATE_COOL; ac.temp = 22;
  auto time = report_time(); time.minute = 3;
  graph_data_update(time, data, sensor, settings, ac);
  assert(data.last_data_update_minute == 0);
  assert(data.temperature[287] == 23 && !graph_valid_value(data.temperature[286]));
  assert(data.temperature_target[143] == 26);
  assert(!graph_valid_value(data.temperature_target[142]));

  // Commands/PID changes within the open slot are visible immediately.
  time.minute = 7;
  settings.ac_auto_temp = 28; settings.hm_target_humidity = 60; ac.temp = 20;
  graph_data_update(time, data, sensor, settings, ac);
  assert(data.temperature_target[143] == 28 && data.ac_set_temperature[143] == 20);
  assert(data.humidity_target[143] == 60);
  assert(!graph_valid_value(data.temperature_target[142]));

  time.minute = 10;
  graph_data_update(time, data, sensor, settings, ac);
  time.minute = 17;
  settings.ac_auto_temp = 24; settings.hm_target_humidity = 45; ac.temp = 21;
  graph_data_update(time, data, sensor, settings, ac);
  assert(data.temperature_target[142] == 28 && data.ac_set_temperature[142] == 20);
  assert(data.humidity_target[142] == 60); // Earlier slots must never be rewritten.
  assert(data.temperature_target[143] == 24 && data.ac_set_temperature[143] == 21);
  assert(data.humidity_target[143] == 45);

  time.minute = 20;
  settings.ac_auto_mode = AC_AUTO_OFF; settings.hm_auto = false; ac.temp = 19;
  graph_data_update(time, data, sensor, settings, ac);
  assert(!graph_valid_value(data.temperature_target[143]));
  assert(!graph_valid_value(data.humidity_target[143]));
  assert(data.ac_set_temperature[143] == 19); // Manual AC retains only its setting.
  assert(data.temperature_target[142] == 24 && data.humidity_target[142] == 45);
  time.minute = 30; ac.state = AC_STATE_OFF;
  graph_data_update(time, data, sensor, settings, ac);
  assert(!graph_valid_value(data.ac_set_temperature[143]));
  assert(data.ac_set_temperature[142] == 19);
  time.minute = 40;
  settings.ac_auto_mode = AC_AUTO_HEAT; settings.ac_auto_temp = 27;
  settings.hm_auto = true; settings.hm_target_humidity = 50;
  ac.state = AC_STATE_HEAT; ac.temp = 29;
  graph_data_update(time, data, sensor, settings, ac);
  assert(data.temperature_target[143] == 27 && data.humidity_target[143] == 50);
  assert(data.ac_set_temperature[143] == 29);
  assert(!graph_valid_value(data.temperature_target[142]));
  assert(!graph_valid_value(data.humidity_target[142]));
  assert(data.temperature_target[140] == 24 && data.humidity_target[140] == 45);

  // A delayed loop crossing a boundary still records in the right ten-minute slot.
  time.minute = 52;
  graph_data_update(time, data, sensor, settings, ac);
  assert(data.last_data_update_minute == 50);
  assert(data.temperature_target[142] == 27);
}

static void test_time_rollover_and_gaps() {
  Graph_data data;
  Settings settings; settings.hm_auto = true; settings.hm_target_humidity = 55;
  AC_status ac;
  Sensor_data sensor; sensor.representative.humidity = 45;
  auto time = report_time(); time.hour = 23; time.minute = 57;
  graph_data_update(time, data, sensor, settings, ac);
  time.day += 1; time.hour = 0; time.minute = 3;
  settings.hm_target_humidity = 60;
  graph_data_update(time, data, sensor, settings, ac);
  assert(data.humidity_target[142] == 55 && data.humidity_target[143] == 60);
  assert(data.humidity[286] == 45 && data.humidity[287] == 45);
  assert(data.last_data_update_hour == 0 && data.last_data_update_minute == 0);

  time.minute = 32;
  graph_data_update(time, data, sensor, settings, ac);
  assert(data.humidity_target[140] == 60 && data.humidity_target[143] == 60);
  assert(!graph_valid_value(data.humidity_target[141]));
  assert(!graph_valid_value(data.humidity_target[142]));
  assert(!graph_valid_value(data.humidity[285]));
  assert(!graph_valid_value(data.humidity[286]));

  time.day += 1;
  graph_data_update(time, data, sensor, settings, ac);
  for (int i = 0; i < GRAPH_DATA_1_DAY - 1; ++i) assert(!graph_valid_value(data.humidity_target[i]));
  assert(data.humidity_target[143] == 60);
  assert(data.humidity[143] == 45); // Previous day's measured trace is still retained.

  time.minute = 12; // A backward clock correction must not reuse misdated history.
  graph_data_update(time, data, sensor, settings, ac);
  for (int i = 0; i < GRAPH_DATA_N - 1; ++i) assert(!graph_valid_value(data.humidity[i]));
  assert(data.humidity_target[143] == 60);
}

static void test_steps_and_off_gaps() {
  Graph_data data;
  for (int x = 0; x < GRAPH_DATA_1_DAY; ++x) {
    data.temperature_target[x] = x < 32 ? 26 : x < 64 ? 28 : x < 80 ? GRAPH_DATA_UNDEFINED : 25;
  }
  GuardedImage guarded;
  graph_draw_white(guarded.image);
  graph_draw_reference(guarded.image, 20, 30, data.temperature_target, true);
  const int old_y = GRAPH_SY + 72; // 26 C
  const int new_y = GRAPH_SY + 96; // 28 C
  for (int y = old_y; y <= new_y; ++y) assert(guarded.image.graph[y][GRAPH_SX + 32] == PALETTE_PURPLE);
  assert(guarded.image.graph[old_y + 12][GRAPH_SX + 31] == PALETTE_WHITE); // No diagonal interpolation.
  for (int x = 64; x < 80; ++x) {
    for (int y = GRAPH_SY; y <= GRAPH_SY + GRAPH_AREA_HEIGHT; ++y) {
      assert(guarded.image.graph[y][GRAPH_SX + x] == PALETTE_WHITE);
    }
  }
  assert(guarded.image.graph[GRAPH_SY + 60][GRAPH_SX + 80] == PALETTE_PURPLE);
  assert(guarded.image.graph[new_y][GRAPH_SX + 80] == PALETTE_WHITE); // Resume does not bridge the gap.
  guarded.check();

  graph_draw_white(guarded.image);
  std::fill_n(data.temperature_target, GRAPH_DATA_1_DAY, GRAPH_DATA_UNDEFINED);
  data.temperature_target[143] = 26;
  graph_draw_reference(guarded.image, 20, 30, data.temperature_target, true);
  graph_draw_reference(guarded.image, 20, 30, data.temperature_target, false);
  assert(guarded.image.graph[old_y][GRAPH_SX + 143] == PALETTE_BLUE);
  assert(guarded.image.graph[old_y + 1][GRAPH_SX + 143] == PALETTE_PURPLE);
  guarded.check();
}

int main(int argc, char **argv) {
  test_control_modes();
  test_temperature_lines();
  test_humidity_and_empty_history();
  test_missing_sample_and_day_boundary();
  test_setting_history();
  test_time_rollover_and_gaps();
  test_steps_and_off_gaps();
  if (argc == 2) {
    auto data = history(); auto time = report_time();
    for (int x = 0; x < GRAPH_DATA_1_DAY; ++x) {
      data.temperature_target[x] = x < 36 ? 26 : x < 72 ? 24 : x < 96 ? GRAPH_DATA_UNDEFINED : 25;
      data.humidity_target[x] = x < 36 ? 55 : x < 72 ? 60 : x < 96 ? GRAPH_DATA_UNDEFINED : 50;
      data.ac_set_temperature[x] = x < 36 ? 20 : x < 72 ? 22 : x < 96 ? GRAPH_DATA_UNDEFINED : 21;
    }
    Graph_img image;
    graph_draw_temperature(data, image, time);
    save_ppm(image, std::string(argv[1]) + "/temperature.ppm");
    graph_draw_humidity(data, image, time);
    save_ppm(image, std::string(argv[1]) + "/humidity.ppm");
  }
  std::cout << "All graph tests passed.\n";
}
