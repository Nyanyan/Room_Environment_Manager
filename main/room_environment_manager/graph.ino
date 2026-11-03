#include "graph.h"
#include "sensors.h"
#include "character.h"
#include "air_conditioner.h"
#include "command.h"
#include "time_manager.h"
#include <math.h>
#include <limits.h>
#include <time.h>

const uint8_t color_palette[BMP_N_COLOR_PALETTE][3] = { // RGB
  {255, 255, 255}, // white
  {  0,   0,   0}, // black
  {127, 127, 127}, // gray
  {200, 200, 200}, // light gray
  {255,   0,   0}, // red
  { 80, 230,  78}, // green
  { 54, 194, 206}, // sky blue
  {  0,   0, 255}, // blue
  {255, 178,  44}, // orange
  {250, 255, 175}, // yellow
  {187, 233, 255}, // light blue
  {220,   0, 151}, // purple
  { 67,  61, 139}, // navy blue
  { 46,  35, 108}  // dark blue
};

const Value_color color_temperature[N_COLOR_TEMPERATURE] = {
  {5, PALETTE_NAVYBLUE},
  {10, PALETTE_BLUE},
  {15, PALETTE_SKYBLUE},
  {20, PALETTE_GREEN},
  {25, PALETTE_ORANGE},
  {30, PALETTE_RED},
  {35, PALETTE_PURPLE},
};

const Value_color color_humidity[N_COLOR_HUMIDITY] = {
  {0, PALETTE_NAVYBLUE},
  {20, PALETTE_BLUE},
  {40, PALETTE_GREEN},
  {60, PALETTE_ORANGE},
  {80, PALETTE_RED},
  {100, PALETTE_PURPLE}
};

const Value_color color_pressure[N_COLOR_PRESSURE] = {
  {990, PALETTE_BLUE},
  {1000, PALETTE_GREEN},
  {1010, PALETTE_ORANGE},
  {1020, PALETTE_RED}
};

const Value_color color_co2_concentration[N_COLOR_CO2_CONCENTRATION] = {
  {400, PALETTE_NAVYBLUE},
  {500, PALETTE_BLUE},
  {600, PALETTE_SKYBLUE},
  {700, PALETTE_GREEN},
  {800, PALETTE_ORANGE},
  {900, PALETTE_RED},
  {1000, PALETTE_PURPLE}
};

void init_graph(Graph_img &graph_img){
  // no initialization needed for JPEG
  (void)graph_img;
}

void graph_draw_white(Graph_img &graph_img) {
  for (int y = 0; y < GRAPH_IMG_HEIGHT; ++y){
    for (int x = 0; x < GRAPH_IMG_WIDTH; ++x){
      graph_img.graph[y][x] = PALETTE_WHITE;
   }
  }
}

void graph_draw_char(Graph_img &graph_img, int ey, int ex, const bool char_data[CHAR_HEIGHT][CHAR_WIDTH]) {
  for (int y = 0; y < CHAR_HEIGHT; ++y) {
    for (int x = 0; x < CHAR_WIDTH; ++x) {
      if (char_data[y][x]) {
        graph_img.graph[ey + CHAR_HEIGHT - 1 - y][ex - CHAR_WIDTH + 1 + x] = PALETTE_BLACK;
      }
    }
  }
}

int graph_draw_int(Graph_img &graph_img, int ey, int ex, int num) {
  if (num == 0) {
    graph_draw_char(graph_img, ey, ex, char_list[0]);
    ex -= CHAR_WIDTH + CHAR_SPACE;
    return ex;
  }
  while (num) {
    int digit = num % 10;
    graph_draw_char(graph_img, ey, ex, char_list[digit]);
    ex -= CHAR_WIDTH + CHAR_SPACE;
    num /= 10;
  }
  return ex;
}

int graph_draw_int_digit(Graph_img &graph_img, int ey, int ex, int num, int n_digit) {
  for (int i = 0; i < n_digit; ++i) {
    int digit = num % 10;
    graph_draw_char(graph_img, ey, ex, char_list[digit]);
    ex -= CHAR_WIDTH + CHAR_SPACE;
    num /= 10;
  }
  return ex;
}

void graph_draw_time(Graph_img &graph_img, Time_info &time_info) {
  // format: YYYY/MM/DD hh:mm
  int ex = GRAPH_IMG_WIDTH - CHAR_TIME_MARGIN_X;
  int ey = GRAPH_IMG_HEIGHT - CHAR_HEIGHT - CHAR_TIME_MARGIN_Y;
  ex = graph_draw_int_digit(graph_img, ey, ex, time_info.minute, 2); // print mm
  graph_draw_char(graph_img, ey, ex, char_list[CHAR_LIST_COLON]); // print :
  ex -= CHAR_WIDTH + CHAR_SPACE;
  ex = graph_draw_int_digit(graph_img, ey, ex, time_info.hour, 2); // print hh
  ex -= CHAR_WIDTH + CHAR_SPACE; // print space
  ex = graph_draw_int_digit(graph_img, ey, ex, time_info.day, 2); // print DD
  graph_draw_char(graph_img, ey, ex, char_list[CHAR_LIST_SLASH]); // print /
  ex -= CHAR_WIDTH + CHAR_SPACE;
  ex = graph_draw_int_digit(graph_img, ey, ex, time_info.month, 2); // print MM
  graph_draw_char(graph_img, ey, ex, char_list[CHAR_LIST_SLASH]); // print /
  ex -= CHAR_WIDTH + CHAR_SPACE;
  ex = graph_draw_int_digit(graph_img, ey, ex, time_info.year, 4); // print YYYY
}

void graph_draw_str(Graph_img &graph_img, int ey, int ex, const int str[], int len_str) {
  for (int i = len_str - 1; i >= 0; --i) {
    graph_draw_char(graph_img, ey, ex, char_list[str[i]]);
    ex -= CHAR_WIDTH + CHAR_SPACE;
  }
}


void graph_draw_title(Graph_img &graph_img, const int str[], int len_str) {
  int ex = len_str * CHAR_WIDTH + (len_str - 1) * CHAR_SPACE + CHAR_TITLE_MARGIN_X;
  int ey = GRAPH_IMG_HEIGHT - CHAR_HEIGHT - CHAR_TITLE_MARGIN_Y;
  for (int i = len_str - 1; i >= 0; --i) {
    graph_draw_char(graph_img, ey, ex, char_list[str[i]]);
    ex -= CHAR_WIDTH + CHAR_SPACE;
  }
}


void graph_draw_x_scale(Graph_img &graph_img, Graph_data &graph_data) {
  // x scale + daytime-night coloring
  for (int32_t x = 0; x < GRAPH_DATA_1_DAY; ++x){
    int32_t minute = graph_data.last_data_update_minute + 1440 - (GRAPH_DATA_1_DAY - 1 - x) * GRAPH_DATA_INTERVAL;
    int32_t hour = graph_data.last_data_update_hour + minute / 60;
    int hour_mod = hour % 24;
    // coloring
    if (6 <= hour_mod && hour_mod < 18){ // daytime
      for (int y = 0; y <= GRAPH_AREA_HEIGHT; ++y){
        graph_img.graph[GRAPH_SY + y][GRAPH_SX + x] = PALETTE_YELLOW;
      }
    } else{ // night
      for (int y = 0; y <= GRAPH_AREA_HEIGHT; ++y){
        graph_img.graph[GRAPH_SY + y][GRAPH_SX + x] = PALETTE_LIGHTBLUE;
      }
    }
    // scale
    if (minute % 60 == 0){ // every 1 hour
      graph_img.graph[GRAPH_SY - 2][GRAPH_SX + x] = PALETTE_GRAY;
      if (hour % 3 == 0){ // every 3 hour
        for (int y = 0; y <= GRAPH_AREA_HEIGHT; ++y){
          graph_img.graph[GRAPH_SY + y][GRAPH_SX + x] = PALETTE_LIGHTGRAY;
        }
        // hour as string
        int n_digit = 1;
        if (hour_mod >= 10) {
          n_digit = 2;
        }
        int ex = GRAPH_SX + x + (CHAR_WIDTH * n_digit) / 2;
        int ey = GRAPH_SY - 3 - CHAR_HEIGHT;
        graph_draw_int(graph_img, ey, ex, hour_mod);
      }
    }
  }
}


void graph_draw_y_scale(Graph_img &graph_img, int y_min, int y_max, int interval, const Value_color scale[], int n_scale) {
  for (int32_t t = y_min; t <= y_max; t += interval){
    uint8_t line_color = PALETTE_LIGHTGRAY;
    for (int i = 0; i < n_scale; ++i){
      if (t == scale[i].value){
        line_color = scale[i].color;
        int32_t ey = GRAPH_SY + round((float)GRAPH_AREA_HEIGHT * (scale[i].value - y_min) / (y_max - y_min)) - CHAR_HEIGHT / 2;
        int32_t ex = GRAPH_SX - 4;
        graph_draw_int(graph_img, ey, ex, scale[i].value);
      }
    }
    int32_t y = round((float)GRAPH_AREA_HEIGHT * (t - y_min) / (y_max - y_min));
    for (int32_t x = 0; x <= GRAPH_AREA_WIDTH; ++x){
      graph_img.graph[GRAPH_SY + y][GRAPH_SX + x] = line_color;
    }
  }
}


void graph_draw_frame(Graph_img &graph_img) {
  // y = y_min
  for (int32_t x = -1; x <= GRAPH_AREA_WIDTH; ++x){
    graph_img.graph[GRAPH_SY - 1][GRAPH_SX + x] = PALETTE_GRAY;
  }
  
  // y = y_max
  for (int32_t x = -1; x <= GRAPH_AREA_WIDTH; ++x){
    graph_img.graph[GRAPH_SY + GRAPH_AREA_HEIGHT + 1][GRAPH_SX + x] = PALETTE_GRAY;
  }

  // x = 0
  for (int32_t y = -1; y <= GRAPH_AREA_HEIGHT; ++y){
    graph_img.graph[GRAPH_SY + y][GRAPH_SX - 1] = PALETTE_GRAY;
  }

  // x = end
  for (int32_t y = -1; y <= GRAPH_AREA_HEIGHT; ++y){
    graph_img.graph[GRAPH_SY + y][GRAPH_SX + GRAPH_AREA_WIDTH] = PALETTE_GRAY;
  }
}


void graph_draw_graph_line(Graph_img &graph_img, int fy, int fx, int ny, int nx, uint8_t color) {
  int min_y = min(fy, ny);
  int max_y = max(fy, ny);
  for (int y = min_y + 1; y < max_y; ++y) {
    int x = round((float)fx + (float)(y - fy) / (ny - fy) * (nx - fx));
    graph_img.graph[GRAPH_SY + y][GRAPH_SX + x] = color;
  }
}


bool graph_valid_value(float value) {
  return isfinite(value) && value != GRAPH_DATA_UNDEFINED && value != FLT_MAX;
}


Graph_references graph_control_references(const Settings &settings, const AC_status &ac_status) {
  Graph_references references;
  if ((settings.ac_auto_mode == AC_AUTO_COOL || settings.ac_auto_mode == AC_AUTO_DRY ||
       settings.ac_auto_mode == AC_AUTO_HEAT) && isfinite(settings.ac_auto_temp)) {
    // Match the effective target used by ac_auto().
    references.temperature_target = constrain(settings.ac_auto_temp,
      (double)AC_TEMP_LIMIT_MIN, (double)AC_TEMP_LIMIT_MAX);
  }
  if ((ac_status.state == AC_STATE_COOL || ac_status.state == AC_STATE_DRY ||
       ac_status.state == AC_STATE_HEAT) &&
      ac_status.temp >= AC_TEMP_LIMIT_MIN && ac_status.temp <= AC_TEMP_LIMIT_MAX) {
    references.ac_set_temperature = ac_status.temp;
  }
  if (settings.hm_auto && isfinite(settings.hm_target_humidity) &&
      settings.hm_target_humidity >= 0.0 && settings.hm_target_humidity <= 100.0) {
    references.humidity_target = settings.hm_target_humidity;
  }
  return references;
}


int graph_calculate_n_main_scale(int y_min, int y_max, const Value_color scales[], const int n_scales) {
  int n_main_scale = 0;
  for (int i = 0; i < n_scales; ++i){
    if (y_min <= scales[i].value && scales[i].value <= y_max){
      ++n_main_scale;
    }
  }
  return n_main_scale;
}


void graph_calculate_range(const float data[], const Value_color scales[], const int n_scales, const int interval, int *y_min, int *y_max, const float references[], const float other_references[]) {
  float low = FLT_MAX;
  float high = -FLT_MAX;
  const float *series[] = {data, references, other_references};
  const int lengths[] = {GRAPH_DATA_N, GRAPH_DATA_1_DAY, GRAPH_DATA_1_DAY};
  for (int s = 0; s < 3; ++s) {
    if (!series[s]) continue;
    for (int i = 0; i < lengths[s]; ++i) {
      const float value = series[s][i];
      if (graph_valid_value(value)){
        low = min(low, value);
        high = max(high, value);
      }
    }
  }
  // A report can be requested before any sensor samples are available.
  if (low == FLT_MAX) {
    low = scales[0].value;
    high = scales[1].value;
  }
  *y_min = (int)floorf(low / interval) * interval;
  *y_max = (int)ceilf(high / interval) * interval;
  while (graph_calculate_n_main_scale(*y_min, *y_max, scales, n_scales) < 2) {
    int n_sub = INT_MAX;
    int n_add = INT_MAX;
    for (int i = 0; i < n_scales; ++i) {
      if (scales[i].value < *y_min) {
        n_sub = min(n_sub, *y_min - scales[i].value);
      }
      if (scales[i].value > *y_max) {
        n_add = min(n_add, scales[i].value - *y_max);
      }
    }
    if (n_sub < n_add) {
      *y_min -= n_sub;
    } else {
      *y_max += n_add;
    }
  }
  // Keep data/reference lines away from the frame, including integer setpoints.
  if (*y_min >= low || graph_calculate_n_main_scale(*y_min, *y_min, scales, n_scales)) {
    *y_min -= interval;
  }
  if (*y_max <= high || graph_calculate_n_main_scale(*y_max, *y_max, scales, n_scales)) {
    *y_max += interval;
  }
}


void graph_plot(Graph_img &graph_img, float data[], int y_min, int y_max) {
  int32_t fy = GRAPH_DATA_UNDEFINED;
  // yesterday
  for (int32_t x = 0; x < GRAPH_DATA_1_DAY; ++x){
    if (graph_valid_value(data[x])){
      int32_t y = round((float)GRAPH_AREA_HEIGHT * (data[x] - y_min) / (y_max - y_min));
      graph_img.graph[GRAPH_SY + y][GRAPH_SX + x] = PALETTE_GRAY;
      if (fy != GRAPH_DATA_UNDEFINED){
        graph_draw_graph_line(graph_img, fy, x - 1, y, x, PALETTE_GRAY);
      }
      fy = y;
    } else {
      fy = GRAPH_DATA_UNDEFINED;
    }
  }
  fy = GRAPH_DATA_UNDEFINED;
  for (int32_t x = GRAPH_DATA_1_DAY; x < GRAPH_DATA_N; ++x){
    if (graph_valid_value(data[x])){
      int32_t y = round((float)GRAPH_AREA_HEIGHT * (data[x] - y_min) / (y_max - y_min));
      graph_img.graph[GRAPH_SY + y][GRAPH_SX + x - GRAPH_DATA_1_DAY] = PALETTE_BLACK;
      if (fy != GRAPH_DATA_UNDEFINED){
        graph_draw_graph_line(graph_img, fy, x - GRAPH_DATA_1_DAY - 1, y, x - GRAPH_DATA_1_DAY, PALETTE_BLACK);
      }
      fy = y;
    } else {
      fy = GRAPH_DATA_UNDEFINED;
    }
  }
}



void graph_draw_reference_pixel(Graph_img &graph_img, int x, int y, bool target) {
  const uint8_t color = target ? PALETTE_PURPLE : PALETTE_BLUE;
  const int second_y = y < GRAPH_AREA_HEIGHT ? y + 1 : y - 1;
  graph_img.graph[GRAPH_SY + y][GRAPH_SX + x] = color;
  // Keep both colors visible even for a single sample with equal references.
  if (target || graph_img.graph[GRAPH_SY + second_y][GRAPH_SX + x] != PALETTE_PURPLE) {
    graph_img.graph[GRAPH_SY + second_y][GRAPH_SX + x] = color;
  }
}


void graph_draw_reference(Graph_img &graph_img, int y_min, int y_max, const float values[], bool target) {
  if (y_min >= y_max) {
    return;
  }
  int previous_y = -1;
  for (int x = 0; x < GRAPH_DATA_1_DAY; ++x) {
    const float value = values[x];
    if (!graph_valid_value(value) || value < y_min || value > y_max) {
      previous_y = -1; // Off/missing intervals must not be bridged.
      continue;
    }
    const int y = round((float)GRAPH_AREA_HEIGHT * (value - y_min) / (y_max - y_min));
    const bool endpoint = previous_y < 0 || x == GRAPH_DATA_1_DAY - 1 ||
      !graph_valid_value(values[x + 1]) || values[x + 1] != value;
    if (endpoint || previous_y != y || (target ? x % 8 < 4 : x % 4 == 2)) {
      graph_draw_reference_pixel(graph_img, x, y, target);
    }
    if (previous_y >= 0 && previous_y != y) {
      // A setting change is a step at this slot, not an interpolated ramp.
      for (int step_y = min(previous_y, y); step_y <= max(previous_y, y); ++step_y) {
        graph_draw_reference_pixel(graph_img, x, step_y, target);
      }
    }
    previous_y = y;
  }
}


void graph_draw_temperature(Graph_data &graph_data, Graph_img &graph_img, Time_info &time_info) {
  graph_draw_white(graph_img);
  graph_draw_x_scale(graph_img, graph_data);
  int y_min, y_max;
  graph_calculate_range(graph_data.temperature, color_temperature, N_COLOR_TEMPERATURE, GRAPH_TEMPERATURE_SCALE_INTERVAL, &y_min, &y_max, graph_data.temperature_target, graph_data.ac_set_temperature);
  graph_draw_y_scale(graph_img, y_min, y_max, GRAPH_TEMPERATURE_SCALE_INTERVAL, color_temperature, N_COLOR_TEMPERATURE);
  graph_draw_reference(graph_img, y_min, y_max, graph_data.temperature_target, true);
  graph_draw_reference(graph_img, y_min, y_max, graph_data.ac_set_temperature, false);
  graph_plot(graph_img, graph_data.temperature, y_min, y_max);
  graph_draw_frame(graph_img);
  graph_draw_str(graph_img, GRAPH_IMG_HEIGHT - CHAR_HEIGHT - CHAR_TITLE_MARGIN_Y, CHAR_TEMPERATURE_N * (CHAR_WIDTH + CHAR_SPACE) - CHAR_SPACE + CHAR_TITLE_MARGIN_X, char_idx_temperature, CHAR_TEMPERATURE_N); // title + y unit
  graph_draw_str(graph_img, CHAR_UNIT_X_EY, CHAR_UNIT_X_EX, char_idx_time, CHAR_TIME_N); // x unit
  graph_draw_time(graph_img, time_info);
}



void graph_draw_humidity(Graph_data &graph_data, Graph_img &graph_img, Time_info &time_info){
  graph_draw_white(graph_img);
  graph_draw_x_scale(graph_img, graph_data);
  int y_min, y_max;
  graph_calculate_range(graph_data.humidity, color_humidity, N_COLOR_HUMIDITY, GRAPH_HUMIDITY_SCALE_INTERVAL, &y_min, &y_max, graph_data.humidity_target, nullptr);
  graph_draw_y_scale(graph_img, y_min, y_max, GRAPH_HUMIDITY_SCALE_INTERVAL, color_humidity, N_COLOR_HUMIDITY);
  graph_draw_reference(graph_img, y_min, y_max, graph_data.humidity_target, true);
  graph_plot(graph_img, graph_data.humidity, y_min, y_max);
  graph_draw_frame(graph_img);
  graph_draw_str(graph_img, GRAPH_IMG_HEIGHT - CHAR_HEIGHT - CHAR_TITLE_MARGIN_Y, CHAR_HUMIDITY_N * (CHAR_WIDTH + CHAR_SPACE) - CHAR_SPACE + CHAR_TITLE_MARGIN_X, char_idx_humidity, CHAR_HUMIDITY_N); // title + y unit
  graph_draw_str(graph_img, CHAR_UNIT_X_EY, CHAR_UNIT_X_EX, char_idx_time, CHAR_TIME_N); // x unit
  graph_draw_time(graph_img, time_info);
}




void graph_draw_pressure(Graph_data &graph_data, Graph_img &graph_img, Time_info &time_info){
  graph_draw_white(graph_img);
  graph_draw_x_scale(graph_img, graph_data);
  int y_min, y_max;
  graph_calculate_range(graph_data.pressure, color_pressure, N_COLOR_PRESSURE, GRAPH_PRESSURE_SCALE_INTERVAL, &y_min, &y_max, nullptr, nullptr);
  graph_draw_y_scale(graph_img, y_min, y_max, GRAPH_PRESSURE_SCALE_INTERVAL, color_pressure, N_COLOR_PRESSURE);
  graph_plot(graph_img, graph_data.pressure, y_min, y_max);
  graph_draw_frame(graph_img);
  graph_draw_str(graph_img, GRAPH_IMG_HEIGHT - CHAR_HEIGHT - CHAR_TITLE_MARGIN_Y, CHAR_PRESSURE_N * (CHAR_WIDTH + CHAR_SPACE) - CHAR_SPACE + CHAR_TITLE_MARGIN_X, char_idx_pressure, CHAR_PRESSURE_N); // title + y unit
  graph_draw_str(graph_img, CHAR_UNIT_X_EY, CHAR_UNIT_X_EX, char_idx_time, CHAR_TIME_N); // x unit
  graph_draw_time(graph_img, time_info);
}



void graph_draw_co2_concentration(Graph_data &graph_data, Graph_img &graph_img, Time_info &time_info){
  graph_draw_white(graph_img);
  graph_draw_x_scale(graph_img, graph_data);
  int y_min, y_max;
  graph_calculate_range(graph_data.co2_concentration, color_co2_concentration, N_COLOR_CO2_CONCENTRATION, GRAPH_CO2_CONCENTRATION_SCALE_INTERVAL, &y_min, &y_max, nullptr, nullptr);
  graph_draw_y_scale(graph_img, y_min, y_max, GRAPH_CO2_CONCENTRATION_SCALE_INTERVAL, color_co2_concentration, N_COLOR_CO2_CONCENTRATION);
  graph_plot(graph_img, graph_data.co2_concentration, y_min, y_max);
  graph_draw_frame(graph_img);
  graph_draw_str(graph_img, GRAPH_IMG_HEIGHT - CHAR_HEIGHT - CHAR_TITLE_MARGIN_Y, CHAR_CO2_CONCENTRATION_N * (CHAR_WIDTH + CHAR_SPACE) - CHAR_SPACE + CHAR_TITLE_MARGIN_X, char_idx_co2_concentration, CHAR_CO2_CONCENTRATION_N); // title + y unit
  graph_draw_str(graph_img, CHAR_UNIT_X_EY, CHAR_UNIT_X_EX, char_idx_time, CHAR_TIME_N); // x unit
  graph_draw_time(graph_img, time_info);
}



void graph_encode_jpeg(Graph_img &graph_img) {
  JPEGENC jpg;
  JPEGENCODE jpe;

  int rc = jpg.open(graph_img.jpeg_buf, JPEG_GRAPH_BUF_SIZE);
  if (rc != JPEGE_SUCCESS) {
    graph_img.jpeg_size = 0;
    return;
  }

  rc = jpg.encodeBegin(&jpe, GRAPH_IMG_WIDTH, GRAPH_IMG_HEIGHT, JPEGE_PIXEL_RGB888, JPEGE_SUBSAMPLE_444, JPEGE_Q_BEST);
  if (rc != JPEGE_SUCCESS) {
    jpg.close();
    graph_img.jpeg_size = 0;
    return;
  }

  uint8_t mcu[8 * 8 * 3];
  const int mcu_w = jpe.cx;
  const int mcu_h = jpe.cy;
  const int mcu_count_x = (GRAPH_IMG_WIDTH + mcu_w - 1) / mcu_w;
  const int mcu_count_y = (GRAPH_IMG_HEIGHT + mcu_h - 1) / mcu_h;

  for (int my = 0; my < mcu_count_y && rc == JPEGE_SUCCESS; my++) {
    for (int mx = 0; mx < mcu_count_x && rc == JPEGE_SUCCESS; mx++) {
      for (int py = 0; py < mcu_h; py++) {
        for (int px = 0; px < mcu_w; px++) {
          int img_x = mx * mcu_w + px;
          int img_y = GRAPH_IMG_HEIGHT - 1 - (my * mcu_h + py); // graph[] is top-down, JPEG is top-down
          if (img_x >= GRAPH_IMG_WIDTH) img_x = GRAPH_IMG_WIDTH - 1;
          if (img_y < 0) img_y = 0;
          const uint8_t palette_idx = graph_img.graph[img_y][img_x];
          const int mcu_idx = (py * mcu_w + px) * 3;
          // JPEGENC on this target interprets 24bpp bytes as B,G,R order.
          mcu[mcu_idx + 0] = color_palette[palette_idx][2]; // B
          mcu[mcu_idx + 1] = color_palette[palette_idx][1]; // G
          mcu[mcu_idx + 2] = color_palette[palette_idx][0]; // R
        }
      }
      rc = jpg.addMCU(&jpe, mcu, mcu_w * 3);
    }
  }

  graph_img.jpeg_size = (uint32_t)jpg.close();
}




void graph_add_data(Graph_data &graph_data, const Sensor_data &sensor_data, int slots){
  // Shift all series together. Missed intervals remain empty instead of placing
  // old samples/controls at the wrong time after a lengthy network operation.
  for (int i = 0; i < GRAPH_DATA_N; ++i){
    const int source = i + slots;
    graph_data.temperature[i] = source < GRAPH_DATA_N ? graph_data.temperature[source] : GRAPH_DATA_UNDEFINED;
    graph_data.humidity[i] = source < GRAPH_DATA_N ? graph_data.humidity[source] : GRAPH_DATA_UNDEFINED;
    graph_data.pressure[i] = source < GRAPH_DATA_N ? graph_data.pressure[source] : GRAPH_DATA_UNDEFINED;
    graph_data.co2_concentration[i] = source < GRAPH_DATA_N ? graph_data.co2_concentration[source] : GRAPH_DATA_UNDEFINED;
    if (i < GRAPH_DATA_1_DAY) {
      graph_data.temperature_target[i] = source < GRAPH_DATA_1_DAY ? graph_data.temperature_target[source] : GRAPH_DATA_UNDEFINED;
      graph_data.ac_set_temperature[i] = source < GRAPH_DATA_1_DAY ? graph_data.ac_set_temperature[source] : GRAPH_DATA_UNDEFINED;
      graph_data.humidity_target[i] = source < GRAPH_DATA_1_DAY ? graph_data.humidity_target[source] : GRAPH_DATA_UNDEFINED;
    }
  }

  const SensorReading &reading = sensor_data.representative;
  graph_data.temperature[GRAPH_DATA_N - 1] = (reading.temperature == FLT_MAX) ? GRAPH_DATA_UNDEFINED : reading.temperature;
  graph_data.humidity[GRAPH_DATA_N - 1] = (reading.humidity == FLT_MAX) ? GRAPH_DATA_UNDEFINED : reading.humidity;
  graph_data.pressure[GRAPH_DATA_N - 1] = (reading.pressure == FLT_MAX) ? GRAPH_DATA_UNDEFINED : reading.pressure;
  graph_data.co2_concentration[GRAPH_DATA_N - 1] = (reading.co2_concentration == FLT_MAX) ? GRAPH_DATA_UNDEFINED : reading.co2_concentration;
}



void graph_data_update(const Time_info &time_info, Graph_data &graph_data, const Sensor_data &sensor_data, const Settings &settings, const AC_status &ac_status){
  struct tm time = {};
  time.tm_year = time_info.year - 1900;
  time.tm_mon = time_info.month - 1;
  time.tm_mday = time_info.day;
  time.tm_hour = time_info.hour;
  time.tm_min = time_info.minute;
  time.tm_isdst = -1;
  const time_t seconds = mktime(&time);
  if (seconds == (time_t)-1) return;
  const int64_t slot = (int64_t)seconds / (GRAPH_DATA_INTERVAL * 60);
  if (slot != graph_data.last_data_update_slot) {
    int64_t elapsed = graph_data.last_data_update_slot < 0 ? 1 : slot - graph_data.last_data_update_slot;
    if (elapsed < 0 || elapsed > GRAPH_DATA_N) elapsed = GRAPH_DATA_N;
    graph_add_data(graph_data, sensor_data, (int)elapsed);
    graph_data.last_data_update_slot = slot;
    graph_data.last_data_update_minute = time_info.minute / GRAPH_DATA_INTERVAL * GRAPH_DATA_INTERVAL;
    graph_data.last_data_update_hour = time_info.hour;
  }
  // Refresh only the open slot so monitor sees commands/PID changes immediately.
  // Completed slots retain their historical values, including auto/off gaps.
  const Graph_references references = graph_control_references(settings, ac_status);
  graph_data.temperature_target[GRAPH_DATA_1_DAY - 1] = references.temperature_target;
  graph_data.ac_set_temperature[GRAPH_DATA_1_DAY - 1] = references.ac_set_temperature;
  graph_data.humidity_target[GRAPH_DATA_1_DAY - 1] = references.humidity_target;
}














