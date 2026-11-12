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
  std::fill_n(data.ac_state, GRAPH_DATA_1_DAY, refs.ac_state);
  std::fill_n(data.humidity_target, GRAPH_DATA_1_DAY, refs.humidity_target);
}

static void test_control_modes() {
  Settings settings;
  AC_status ac;
  auto refs = graph_control_references(settings, ac);
  assert(!graph_valid_value(refs.temperature_target));
  assert(!graph_valid_value(refs.ac_set_temperature));
  assert(refs.ac_state == AC_STATE_OFF);
  assert(!graph_valid_value(refs.humidity_target));

  for (int mode : {AC_STATE_COOL, AC_STATE_DRY, AC_STATE_HEAT}) {
    ac.state = mode; ac.temp = 22;
    refs = graph_control_references(settings, ac);
    assert(refs.ac_set_temperature == 22);
    assert(refs.ac_state == mode);
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
  assert(refs.ac_state == AC_STATE_OFF);
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
  assert(refs.ac_state == AC_STATE_OFF);
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
  assert(guarded.image.graph[setting_y][GRAPH_SX + 2] == PALETTE_AC_COOL + PALETTE_LIGHTGRAY);
  guarded.check();

  // Equal references blend with the purple target instead of erasing it.
  refs.ac_set_temperature = refs.temperature_target;
  fill_controls(data, refs);
  graph_draw_temperature(data, guarded.image, time);
  int coincident_rows = 0;
  for (int y = GRAPH_SY; y <= GRAPH_SY + GRAPH_AREA_HEIGHT; ++y) {
    if (guarded.image.graph[y][GRAPH_SX + 1] == PALETTE_AC_COOL + PALETTE_PURPLE &&
        guarded.image.graph[y][GRAPH_SX + 2] == PALETTE_AC_COOL + PALETTE_PURPLE) {
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
      assert(guarded.image.graph[y][x] < BMP_N_BASE_COLORS);
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
      uint8_t rgb[3];
      graph_pixel_rgb(image.graph[y][x], rgb);
      file.write(reinterpret_cast<const char *>(rgb), 3);
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
  assert(data.ac_state[143] == AC_STATE_COOL && data.ac_state[142] == AC_STATE_OFF);

  // Commands/PID changes within the open slot are visible immediately.
  time.minute = 7;
  settings.ac_auto_temp = 28; settings.hm_target_humidity = 60; ac.temp = 20; ac.state = AC_STATE_DRY;
  graph_data_update(time, data, sensor, settings, ac);
  assert(data.temperature_target[143] == 28 && data.ac_set_temperature[143] == 20);
  assert(data.humidity_target[143] == 60);
  assert(data.ac_state[143] == AC_STATE_DRY);
  assert(!graph_valid_value(data.temperature_target[142]));

  time.minute = 10;
  graph_data_update(time, data, sensor, settings, ac);
  time.minute = 17;
  settings.ac_auto_temp = 24; settings.hm_target_humidity = 45; ac.temp = 21; ac.state = AC_STATE_COOL;
  graph_data_update(time, data, sensor, settings, ac);
  assert(data.temperature_target[142] == 28 && data.ac_set_temperature[142] == 20);
  assert(data.humidity_target[142] == 60); // Earlier slots must never be rewritten.
  assert(data.temperature_target[143] == 24 && data.ac_set_temperature[143] == 21);
  assert(data.humidity_target[143] == 45);
  assert(data.ac_state[142] == AC_STATE_DRY && data.ac_state[143] == AC_STATE_COOL);

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
  assert(data.ac_state[142] == AC_STATE_COOL && data.ac_state[143] == AC_STATE_OFF);
  time.minute = 40;
  settings.ac_auto_mode = AC_AUTO_HEAT; settings.ac_auto_temp = 27;
  settings.hm_auto = true; settings.hm_target_humidity = 50;
  ac.state = AC_STATE_HEAT; ac.temp = 29;
  graph_data_update(time, data, sensor, settings, ac);
  assert(data.temperature_target[143] == 27 && data.humidity_target[143] == 50);
  assert(data.ac_set_temperature[143] == 29);
  assert(data.ac_state[143] == AC_STATE_HEAT && data.ac_state[142] == AC_STATE_OFF);
  assert(!graph_valid_value(data.temperature_target[142]));
  assert(!graph_valid_value(data.humidity_target[142]));
  assert(data.temperature_target[140] == 24 && data.humidity_target[140] == 45);

  // A delayed loop crossing a boundary still records in the right ten-minute slot.
  time.minute = 52;
  graph_data_update(time, data, sensor, settings, ac);
  assert(data.last_data_update_minute == 50);
  assert(data.temperature_target[142] == 27);
  assert(data.ac_state[142] == AC_STATE_HEAT);
}

static void test_time_rollover_and_gaps() {
  Graph_data data;
  Settings settings; settings.hm_auto = true; settings.hm_target_humidity = 55;
  AC_status ac;
  ac.state = AC_STATE_COOL; ac.temp = 24;
  Sensor_data sensor; sensor.representative.humidity = 45;
  auto time = report_time(); time.hour = 23; time.minute = 57;
  graph_data_update(time, data, sensor, settings, ac);
  time.day += 1; time.hour = 0; time.minute = 3;
  settings.hm_target_humidity = 60;
  ac.state = AC_STATE_HEAT; // Same temperature, different mode across midnight.
  graph_data_update(time, data, sensor, settings, ac);
  assert(data.humidity_target[142] == 55 && data.humidity_target[143] == 60);
  assert(data.ac_state[142] == AC_STATE_COOL && data.ac_state[143] == AC_STATE_HEAT);
  assert(data.ac_set_temperature[142] == 24 && data.ac_set_temperature[143] == 24);
  assert(data.humidity[286] == 45 && data.humidity[287] == 45);
  assert(data.last_data_update_hour == 0 && data.last_data_update_minute == 0);

  time.minute = 32;
  graph_data_update(time, data, sensor, settings, ac);
  assert(data.humidity_target[140] == 60 && data.humidity_target[143] == 60);
  assert(!graph_valid_value(data.humidity_target[141]));
  assert(!graph_valid_value(data.humidity_target[142]));
  assert(!graph_valid_value(data.humidity[285]));
  assert(!graph_valid_value(data.humidity[286]));
  assert(data.ac_state[140] == AC_STATE_HEAT && data.ac_state[143] == AC_STATE_HEAT);
  assert(data.ac_state[141] == AC_STATE_OFF && data.ac_state[142] == AC_STATE_OFF);

  time.day += 1;
  graph_data_update(time, data, sensor, settings, ac);
  for (int i = 0; i < GRAPH_DATA_1_DAY - 1; ++i) assert(!graph_valid_value(data.humidity_target[i]));
  assert(data.humidity_target[143] == 60);
  assert(data.humidity[143] == 45); // Previous day's measured trace is still retained.
  for (int i = 0; i < GRAPH_DATA_1_DAY - 1; ++i) assert(data.ac_state[i] == AC_STATE_OFF);
  assert(data.ac_state[143] == AC_STATE_HEAT);

  time.minute = 12; // A backward clock correction must not reuse misdated history.
  graph_data_update(time, data, sensor, settings, ac);
  for (int i = 0; i < GRAPH_DATA_N - 1; ++i) assert(!graph_valid_value(data.humidity[i]));
  assert(data.humidity_target[143] == 60);
  for (int i = 0; i < GRAPH_DATA_1_DAY - 1; ++i) assert(data.ac_state[i] == AC_STATE_OFF);
}

static void test_steps_and_off_gaps() {
  Graph_data data;
  for (int x = 0; x < GRAPH_DATA_1_DAY; ++x) {
    data.temperature_target[x] = x < 32 ? 26 : x < 64 ? 28 : x < 80 ? GRAPH_DATA_UNDEFINED : 25;
  }
  GuardedImage guarded;
  graph_draw_white(guarded.image);
  graph_draw_reference(guarded.image, 20, 30, data.temperature_target);
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
  graph_draw_reference(guarded.image, 20, 30, data.temperature_target);
  data.ac_set_temperature[143] = 26; data.ac_state[143] = AC_STATE_COOL;
  graph_draw_ac_setting(guarded.image, 20, 30, data);
  assert(guarded.image.graph[old_y][GRAPH_SX + 143] == PALETTE_AC_COOL + PALETTE_PURPLE);
  assert(guarded.image.graph[old_y + 1][GRAPH_SX + 143] == PALETTE_AC_COOL + PALETTE_PURPLE);
  guarded.check();
}

static void test_ac_band_blending() {
  for (int mode : {AC_STATE_COOL, AC_STATE_DRY, AC_STATE_HEAT}) {
    Graph_data data;
    std::fill_n(data.ac_set_temperature, GRAPH_DATA_1_DAY, 25);
    std::fill_n(data.ac_state, GRAPH_DATA_1_DAY, mode);
    GuardedImage guarded;
    graph_draw_white(guarded.image);
    for (int y = 0; y <= GRAPH_AREA_HEIGHT; ++y) {
      for (int x = 0; x < GRAPH_DATA_1_DAY; ++x) {
        guarded.image.graph[GRAPH_SY + y][GRAPH_SX + x] = x % BMP_N_BASE_COLORS;
      }
    }
    graph_draw_ac_setting(guarded.image, 20, 30, data);
    const uint8_t bank = mode == AC_STATE_HEAT ? PALETTE_AC_HEAT : PALETTE_AC_COOL;
    const uint8_t tint[3] = {static_cast<uint8_t>(mode == AC_STATE_HEAT ? 255 : 160),
                            static_cast<uint8_t>(mode == AC_STATE_HEAT ? 160 : 200),
                            static_cast<uint8_t>(mode == AC_STATE_HEAT ? 160 : 255)};
    for (int y = 0; y <= GRAPH_AREA_HEIGHT; ++y) {
      for (int x = 0; x < GRAPH_DATA_1_DAY; ++x) {
        const uint8_t base = x % BMP_N_BASE_COLORS;
        const bool in_band = std::abs(y - 60) <= GRAPH_AC_BAND_RADIUS;
        const uint8_t pixel = guarded.image.graph[GRAPH_SY + y][GRAPH_SX + x];
        assert(pixel == (in_band ? bank + base : base));
        uint8_t rgb[3]; graph_pixel_rgb(pixel, rgb);
        for (int c = 0; c < 3; ++c) {
          const int expected = in_band ? (color_palette[base][c] + tint[c]) / 2 : color_palette[base][c];
          assert(rgb[c] == expected);
        }
      }
    }
    guarded.check();
  }
}

static void test_ac_band_steps_and_gaps() {
  Graph_data data;
  for (int x = 0; x < GRAPH_DATA_1_DAY; ++x) {
    data.ac_set_temperature[x] = x < 32 ? 22 : x < 64 ? 28 : x < 96 ? 24 : 26;
    data.ac_state[x] = x < 64 ? AC_STATE_COOL : x < 80 ? AC_STATE_OFF : AC_STATE_HEAT;
  }
  data.ac_set_temperature[110] = NAN;
  data.ac_set_temperature[111] = INFINITY;
  data.ac_set_temperature[112] = FLT_MAX;
  data.ac_set_temperature[113] = GRAPH_DATA_UNDEFINED;
  data.ac_set_temperature[114] = 31; // Outside the plotted range.
  data.ac_state[115] = 255; // Invalid state.
  data.ac_state[120] = AC_STATE_COOL; data.ac_set_temperature[120] = 22;
  GuardedImage guarded;
  graph_draw_white(guarded.image);
  graph_draw_ac_setting(guarded.image, 20, 30, data);
  for (int y = 24 - GRAPH_AC_BAND_RADIUS; y <= 96 + GRAPH_AC_BAND_RADIUS; ++y) {
    assert(guarded.image.graph[GRAPH_SY + y][GRAPH_SX + 32] == PALETTE_AC_COOL + PALETTE_WHITE);
  }
  assert(guarded.image.graph[GRAPH_SY + 60][GRAPH_SX + 31] == PALETTE_WHITE);
  for (int x = 64; x < 80; ++x) {
    for (int y = 0; y <= GRAPH_AREA_HEIGHT; ++y) {
      assert(guarded.image.graph[GRAPH_SY + y][GRAPH_SX + x] == PALETTE_WHITE);
    }
  }
  assert(guarded.image.graph[GRAPH_SY + 48][GRAPH_SX + 80] == PALETTE_AC_HEAT + PALETTE_WHITE);
  assert(guarded.image.graph[GRAPH_SY + 96][GRAPH_SX + 80] == PALETTE_WHITE);
  for (int y = 48 - GRAPH_AC_BAND_RADIUS; y <= 72 + GRAPH_AC_BAND_RADIUS; ++y) {
    assert(guarded.image.graph[GRAPH_SY + y][GRAPH_SX + 96] == PALETTE_AC_HEAT + PALETTE_WHITE);
  }
  for (int x = 110; x <= 115; ++x) {
    for (int y = 0; y <= GRAPH_AREA_HEIGHT; ++y) {
      assert(guarded.image.graph[GRAPH_SY + y][GRAPH_SX + x] == PALETTE_WHITE);
    }
  }
  assert(guarded.image.graph[GRAPH_SY + 24][GRAPH_SX + 120] == PALETTE_AC_COOL + PALETTE_WHITE);
  assert(guarded.image.graph[GRAPH_SY + 48][GRAPH_SX + 120] == PALETTE_WHITE); // No connector across modes.
  guarded.check();

  // Clip the ribbon at the top/bottom, and keep a lone last sample visible.
  Graph_data edges;
  edges.ac_set_temperature[0] = 20; edges.ac_state[0] = AC_STATE_COOL;
  edges.ac_set_temperature[143] = 30; edges.ac_state[143] = AC_STATE_HEAT;
  graph_draw_white(guarded.image);
  graph_draw_ac_setting(guarded.image, 20, 30, edges);
  for (int y = 0; y <= GRAPH_AC_BAND_RADIUS; ++y) {
    assert(guarded.image.graph[GRAPH_SY + y][GRAPH_SX] == PALETTE_AC_COOL + PALETTE_WHITE);
    assert(guarded.image.graph[GRAPH_SY + GRAPH_AREA_HEIGHT - y][GRAPH_SX + 143] == PALETTE_AC_HEAT + PALETTE_WHITE);
  }
  assert(guarded.image.graph[GRAPH_SY - 1][GRAPH_SX] == PALETTE_WHITE);
  assert(guarded.image.graph[GRAPH_SY + GRAPH_AREA_HEIGHT + 1][GRAPH_SX + 143] == PALETTE_WHITE);
  guarded.check();
  graph_draw_white(guarded.image);
  graph_draw_ac_setting(guarded.image, 30, 20, edges);
  graph_draw_ac_setting(guarded.image, 20, 20, edges);
  for (const auto &row : guarded.image.graph) for (uint8_t pixel : row) assert(pixel == PALETTE_WHITE);
  guarded.check();
}

static void test_ac_band_over_sensor_traces() {
  auto data = history(); auto time = report_time();
  std::fill_n(data.ac_set_temperature, GRAPH_DATA_1_DAY, 24);
  std::fill_n(data.temperature_target, GRAPH_DATA_1_DAY, 24);
  Graph_img baseline;
  graph_draw_temperature(data, baseline, time);
  std::fill_n(data.ac_state, GRAPH_DATA_1_DAY, AC_STATE_HEAT);
  GuardedImage guarded;
  graph_draw_temperature(data, guarded.image, time);
  int underlying_counts[BMP_N_BASE_COLORS] = {};
  for (int y = 0; y < GRAPH_IMG_HEIGHT; ++y) {
    for (int x = 0; x < GRAPH_IMG_WIDTH; ++x) {
      const uint8_t pixel = guarded.image.graph[y][x];
      if (pixel >= BMP_N_BASE_COLORS) {
        assert(pixel == PALETTE_AC_HEAT + baseline.graph[y][x]);
        ++underlying_counts[baseline.graph[y][x]];
      } else {
        assert(pixel == baseline.graph[y][x]);
      }
    }
  }
  assert(underlying_counts[PALETTE_BLACK] > 0);
  assert(underlying_counts[PALETTE_GRAY] > 0);
  assert(underlying_counts[PALETTE_PURPLE] > 0);
  assert(underlying_counts[PALETTE_LIGHTGRAY] > 0);
  assert(underlying_counts[PALETTE_YELLOW] > 0);
  assert(underlying_counts[PALETTE_LIGHTBLUE] > 0);
  guarded.check();
}

static void test_jpeg_overlay_colors() {
  Graph_img image;
  for (int y = 0; y < GRAPH_IMG_HEIGHT; ++y) {
    for (int x = 0; x < GRAPH_IMG_WIDTH; ++x) {
      image.graph[y][x] = (y * GRAPH_IMG_WIDTH + x) % BMP_N_COLOR_PALETTE;
    }
  }
  graph_encode_jpeg(image);
  assert(image.jpeg_size == 128);
  const int tiles_x = (GRAPH_IMG_WIDTH + 7) / 8;
  const int tiles_y = (GRAPH_IMG_HEIGHT + 7) / 8;
  assert(jpeg_test_mcus.size() == static_cast<size_t>(tiles_x * tiles_y * 64 * 3));
  for (int out_y = 0; out_y < tiles_y * 8; ++out_y) {
    for (int out_x = 0; out_x < tiles_x * 8; ++out_x) {
      const int x = min(out_x, GRAPH_IMG_WIDTH - 1);
      const int y = max(0, GRAPH_IMG_HEIGHT - 1 - out_y);
      const int pixel = image.graph[y][x];
      const int base = pixel % BMP_N_BASE_COLORS;
      const bool cool = pixel >= PALETTE_AC_COOL && pixel < PALETTE_AC_HEAT;
      const bool heat = pixel >= PALETTE_AC_HEAT;
      const int tint[3] = {heat ? 255 : 160, heat ? 160 : 200, heat ? 160 : 255};
      const size_t offset = ((out_y / 8 * tiles_x + out_x / 8) * 64 + (out_y % 8 * 8 + out_x % 8)) * 3;
      for (int c = 0; c < 3; ++c) {
        const int expected = cool || heat ? (color_palette[base][c] + tint[c]) / 2 : color_palette[base][c];
        assert(jpeg_test_mcus[offset + 2 - c] == expected);
      }
    }
  }
}

int main(int argc, char **argv) {
  test_control_modes();
  test_temperature_lines();
  test_humidity_and_empty_history();
  test_missing_sample_and_day_boundary();
  test_setting_history();
  test_time_rollover_and_gaps();
  test_steps_and_off_gaps();
  test_ac_band_blending();
  test_ac_band_steps_and_gaps();
  test_ac_band_over_sensor_traces();
  test_jpeg_overlay_colors();
  if (argc == 2) {
    auto data = history(); auto time = report_time();
    for (int x = 0; x < GRAPH_DATA_1_DAY; ++x) {
      data.temperature_target[x] = x < 36 ? 26 : x < 72 ? 24 : x < 96 ? GRAPH_DATA_UNDEFINED : 25;
      data.humidity_target[x] = x < 36 ? 55 : x < 72 ? 60 : x < 96 ? GRAPH_DATA_UNDEFINED : 50;
      data.ac_set_temperature[x] = x < 36 ? 23 : x < 72 ? 24 : x < 96 ? GRAPH_DATA_UNDEFINED : x < 120 ? 24 : 25;
      data.ac_state[x] = x < 72 ? AC_STATE_COOL : x < 96 ? AC_STATE_OFF : AC_STATE_HEAT;
    }
    Graph_img image;
    graph_draw_temperature(data, image, time);
    save_ppm(image, std::string(argv[1]) + "/temperature.ppm");
    graph_draw_humidity(data, image, time);
    save_ppm(image, std::string(argv[1]) + "/humidity.ppm");
    std::fill_n(data.temperature_target, GRAPH_DATA_1_DAY, 24);
    std::fill_n(data.ac_set_temperature, GRAPH_DATA_1_DAY, 24);
    for (int x = 0; x < GRAPH_DATA_1_DAY; ++x) {
      data.ac_state[x] = x < 72 ? AC_STATE_COOL : AC_STATE_HEAT;
    }
    graph_draw_temperature(data, image, time);
    save_ppm(image, std::string(argv[1]) + "/temperature-overlap.ppm");
  }
  std::cout << "All graph tests passed.\n";
}
