#include "graph.h"
#include "character.h"

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

const Value_color color_thi[N_COLOR_THI] = {
  {50, PALETTE_BLUE},
  {60, PALETTE_SKYBLUE},
  {70, PALETTE_GREEN},
  {80, PALETTE_ORANGE},
  {90, PALETTE_RED}
};

void init_graph(Graph_img &graph_img){
  uint16_t* bmp_img_16;            // bmp header as uint16_t
  uint32_t* bmp_img_32;            // bmp header as uint32_t

  // file header
  graph_img.bmp_img[0] = 'B';  // bfType: always B
  graph_img.bmp_img[1] = 'M';  // bfType: always M
  bmp_img_32 = (uint32_t*)(graph_img.bmp_img + 2);
  bmp_img_32[0] = BMP_GRAPH_FILE_SIZE;           // bfSize: file size
  bmp_img_32[1] = 0;                       // bfReserved1 & bfReserved2: always 0
  bmp_img_32[2] = BMP_OFFSET_TO_IMG_DATA;  // bfOffBits: offset to main image data

  // information header
  bmp_img_32[3] = 40;                     // bcSize: 40 bytes
  bmp_img_32[4] = GRAPH_IMG_WIDTH;        // bcWidth: image width
  bmp_img_32[5] = GRAPH_IMG_HEIGHT;       // bcHeight: image height
  bmp_img_16 = (uint16_t*)(graph_img.bmp_img + 26);
  bmp_img_16[0] = 1;                      // bcPlanes: always 1
  bmp_img_16[1] = BMP_BIT_PER_PIXEL;      // bcBitCount: bit per pixel
  bmp_img_32 = (uint32_t*)(graph_img.bmp_img + 30);
  bmp_img_32[0] = 0;                      // biCompression: compression, 0 = no compression
  bmp_img_32[1] = 3780;                   // biSizeImage: 3780 = 96 dpi
  bmp_img_32[2] = 3780;                   // biXPixPerMeter: 3780 = 96 dpi
  bmp_img_32[3] = 3780;                   // biYPixPerMeter: 3780 = 96 dpi
  bmp_img_32[4] = BMP_N_COLOR_PALETTE;    // biClrUsed: color palette size
  bmp_img_32[5] = BMP_N_COLOR_PALETTE;    // biCirImportant: important color palette size

  // color palette
  for (int i = 0; i < BMP_N_COLOR_PALETTE; ++i){
    for (int j = 0; j < 3; ++j){
      graph_img.bmp_img[BMP_HEADER_BYTE + 4 * i + 2 - j] = color_palette[i][j]; // note: BGR
    }
    graph_img.bmp_img[BMP_HEADER_BYTE + 4 * i + 3] = 0;
  }
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


int graph_calculate_n_main_scale(int y_min, int y_max, const Value_color scales[], const int n_scales) {
  int n_main_scale = 0;
  for (int32_t t = y_min; t <= y_max; ++t){
    for (int i = 0; i < n_scales; ++i){
      if (t == scales[i].value){
        ++n_main_scale;
      }
    }
  }
  return n_main_scale;
}


void graph_calculate_range(float data[], const Value_color scales[], const int n_scales, const int interval, int *y_min, int *y_max) {
  *y_min = 10000;
  *y_max = -10000;
  for (int i = 0; i < GRAPH_DATA_N; ++i){
    if (data[i] != GRAPH_DATA_UNDEFINED){
      *y_min = min(*y_min, (int)(data[i] - 0.9999));
      *y_max = max(*y_max, (int)(data[i] + 0.9999));
    }
  }
  *y_min -= *y_min % interval;
  *y_max += (interval - *y_max % interval) % interval;
  while (graph_calculate_n_main_scale(*y_min, *y_max, scales, n_scales) < 2) {
    int n_sub, n_add;
    for (n_sub = 1; n_sub < 200; ++n_sub) {
      if (graph_calculate_n_main_scale(*y_min - n_sub, *y_min - n_sub, scales, n_scales)) {
        break;
      }
    }
    for (n_add = 1; n_add < 200; ++n_add) {
      if (graph_calculate_n_main_scale(*y_max + n_add, *y_max + n_add, scales, n_scales)) {
        break;
      }
    }
    if (n_sub < n_add) {
      *y_min -= n_sub;
    } else {
      *y_max += n_add;
    }
  }
  if (graph_calculate_n_main_scale(*y_min, *y_min, scales, n_scales)) {
    *y_min -= interval;
  }
  if (graph_calculate_n_main_scale(*y_max, *y_max, scales, n_scales)) {
    *y_max += interval;
  }
}


void graph_plot(Graph_img &graph_img, float data[], int y_min, int y_max) {
  int32_t fy = GRAPH_DATA_UNDEFINED;
  // yesterday
  for (int32_t x = 0; x < GRAPH_DATA_1_DAY; ++x){
    if (data[x] != GRAPH_DATA_UNDEFINED){
      int32_t y = round((float)GRAPH_AREA_HEIGHT * (data[x] - y_min) / (y_max - y_min));
      graph_img.graph[GRAPH_SY + y][GRAPH_SX + x] = PALETTE_GRAY;
      if (fy != GRAPH_DATA_UNDEFINED){
        graph_draw_graph_line(graph_img, fy, x - 1, y, x, PALETTE_GRAY);
      }
      fy = y;
    }
  }
  for (int32_t x = GRAPH_DATA_1_DAY; x < GRAPH_DATA_N; ++x){
    if (data[x] != GRAPH_DATA_UNDEFINED){
      int32_t y = round((float)GRAPH_AREA_HEIGHT * (data[x] - y_min) / (y_max - y_min));
      graph_img.graph[GRAPH_SY + y][GRAPH_SX + x - GRAPH_DATA_1_DAY] = PALETTE_BLACK;
      if (fy != GRAPH_DATA_UNDEFINED){
        graph_draw_graph_line(graph_img, fy, x - GRAPH_DATA_1_DAY - 1, y, x - GRAPH_DATA_1_DAY, PALETTE_BLACK);
      }
      fy = y;
    }
  }
}



void graph_draw_temperature(Graph_data &graph_data, Graph_img &graph_img, Time_info &time_info) {
  graph_draw_white(graph_img);
  graph_draw_x_scale(graph_img, graph_data);
  int y_min, y_max;
  graph_calculate_range(graph_data.temperature, color_temperature, N_COLOR_TEMPERATURE, GRAPH_TEMPERATURE_SCALE_INTERVAL, &y_min, &y_max);
  graph_draw_y_scale(graph_img, y_min, y_max, GRAPH_TEMPERATURE_SCALE_INTERVAL, color_temperature, N_COLOR_TEMPERATURE);
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
  graph_calculate_range(graph_data.humidity, color_humidity, N_COLOR_HUMIDITY, GRAPH_HUMIDITY_SCALE_INTERVAL, &y_min, &y_max);
  graph_draw_y_scale(graph_img, y_min, y_max, GRAPH_HUMIDITY_SCALE_INTERVAL, color_humidity, N_COLOR_HUMIDITY);
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
  graph_calculate_range(graph_data.pressure, color_pressure, N_COLOR_PRESSURE, GRAPH_PRESSURE_SCALE_INTERVAL, &y_min, &y_max);
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
  graph_calculate_range(graph_data.co2_concentration, color_co2_concentration, N_COLOR_CO2_CONCENTRATION, GRAPH_CO2_CONCENTRATION_SCALE_INTERVAL, &y_min, &y_max);
  graph_draw_y_scale(graph_img, y_min, y_max, GRAPH_CO2_CONCENTRATION_SCALE_INTERVAL, color_co2_concentration, N_COLOR_CO2_CONCENTRATION);
  graph_plot(graph_img, graph_data.co2_concentration, y_min, y_max);
  graph_draw_frame(graph_img);
  graph_draw_str(graph_img, GRAPH_IMG_HEIGHT - CHAR_HEIGHT - CHAR_TITLE_MARGIN_Y, CHAR_CO2_CONCENTRATION_N * (CHAR_WIDTH + CHAR_SPACE) - CHAR_SPACE + CHAR_TITLE_MARGIN_X, char_idx_co2_concentration, CHAR_CO2_CONCENTRATION_N); // title + y unit
  graph_draw_str(graph_img, CHAR_UNIT_X_EY, CHAR_UNIT_X_EX, char_idx_time, CHAR_TIME_N); // x unit
  graph_draw_time(graph_img, time_info);
}



void graph_draw_thi(Graph_data &graph_data, Graph_img &graph_img, Time_info &time_info){
  graph_draw_white(graph_img);
  graph_draw_x_scale(graph_img, graph_data);
  int y_min, y_max;
  graph_calculate_range(graph_data.thi, color_thi, N_COLOR_THI, GRAPH_THI_SCALE_INTERVAL, &y_min, &y_max);
  graph_draw_y_scale(graph_img, y_min, y_max, GRAPH_THI_SCALE_INTERVAL, color_thi, N_COLOR_THI);
  graph_plot(graph_img, graph_data.thi, y_min, y_max);
  graph_draw_frame(graph_img);
  graph_draw_str(graph_img, GRAPH_IMG_HEIGHT - CHAR_HEIGHT - CHAR_TITLE_MARGIN_Y, CHAR_THI_N * (CHAR_WIDTH + CHAR_SPACE) - CHAR_SPACE + CHAR_TITLE_MARGIN_X, char_idx_thi, CHAR_THI_N); // title + y unit
  graph_draw_str(graph_img, CHAR_UNIT_X_EY, CHAR_UNIT_X_EX, char_idx_time, CHAR_TIME_N); // x unit
  graph_draw_time(graph_img, time_info);
}



void graph_encode_bmp(Graph_img &graph_img){
  // bitmap init
  for (int i = BMP_OFFSET_TO_IMG_DATA; i < BMP_GRAPH_FILE_SIZE; ++i){
    graph_img.bmp_img[i] = 0;
  }

  // graph to bitmap
  int bmp_idx = BMP_OFFSET_TO_IMG_DATA;
  int n_pixel_per_elem = 0;
  for (int y = 0; y < GRAPH_IMG_HEIGHT; ++y){
    for (int x = 0; x < GRAPH_IMG_WIDTH; ++x){
      if (0 < n_pixel_per_elem){
        graph_img.bmp_img[bmp_idx] <<= BMP_BIT_PER_PIXEL;
      }
      graph_img.bmp_img[bmp_idx] |= graph_img.graph[y][x];
      ++n_pixel_per_elem;
      if (n_pixel_per_elem == 8 / BMP_BIT_PER_PIXEL){
        ++bmp_idx;
        n_pixel_per_elem = 0;
      }
    }
  }
}




void graph_add_data(Graph_data &graph_data, Sensor_data &sensor_data){
  for (int i = 0; i < GRAPH_DATA_N - 1; ++i){
    graph_data.temperature[i] = graph_data.temperature[i + 1];
    graph_data.humidity[i] = graph_data.humidity[i + 1];
    graph_data.pressure[i] = graph_data.pressure[i + 1];
    graph_data.co2_concentration[i] = graph_data.co2_concentration[i + 1];
    graph_data.thi[i] = graph_data.thi[i + 1];
  }
  graph_data.temperature[GRAPH_DATA_N - 1] = sensor_data.temperature;
  graph_data.humidity[GRAPH_DATA_N - 1] = sensor_data.humidity;
  graph_data.pressure[GRAPH_DATA_N - 1] = sensor_data.pressure;
  graph_data.co2_concentration[GRAPH_DATA_N - 1] = sensor_data.co2_concentration;
  graph_data.thi[GRAPH_DATA_N - 1] = sensor_calc_thi(sensor_data.temperature, sensor_data.humidity);
}



void graph_data_update(Time_info &time_info, Graph_data &graph_data, Sensor_data &sensor_data){
  if (time_info.minute != graph_data.last_data_update_minute && time_info.minute % GRAPH_DATA_INTERVAL == 0){
    graph_data.last_data_update_minute = time_info.minute;
    graph_data.last_data_update_hour = time_info.hour;
    graph_add_data(graph_data, sensor_data);
  }
}














