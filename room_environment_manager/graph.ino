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
  { 67,  61, 139}  // navy blue
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
  {20, PALETTE_BLUE},
  {40, PALETTE_GREEN},
  {60, PALETTE_ORANGE},
  {80, PALETTE_RED}
};

const Value_color color_pressure[N_COLOR_PRESSURE] = {
  {980, PALETTE_BLUE},
  {990, PALETTE_SKYBLUE},
  {1000, PALETTE_GREEN},
  {1010, PALETTE_ORANGE},
  {1020, PALETTE_RED}
};

const Value_color color_co2_concentration[N_COLOR_CO2_CONCENTRATION] = {
  {500, PALETTE_BLUE},
  {600, PALETTE_SKYBLUE},
  {700, PALETTE_GREEN},
  {800, PALETTE_ORANGE},
  {900, PALETTE_RED},
  {1000, PALETTE_PURPLE},
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

void graph_draw_time(Graph_img &graph_img, Time_info &time_info) {
  // format: YYYY/MM/DD hh:mm
  int ex = GRAPH_IMG_WIDTH - CHAR_TIME_MARGIN_X;
  int ey = CHAR_TIME_MARGIN_Y;
  // print mm
  for (int i = 0; i < 2; ++i) {
    int digit = time_info.time_str[4 - i] - '0';
    graph_draw_char(graph_img, ey, ex, char_digit[digit]);
    ex -= CHAR_WIDTH + CHAR_SPACE;
  }
  // print :
  graph_draw_char(graph_img, ey, ex, char_colon);
  ex -= CHAR_WIDTH + CHAR_SPACE;
  // print hh
  for (int i = 0; i < 2; ++i) {
    int digit = time_info.time_str[1 - i] - '0';
    graph_draw_char(graph_img, ey, ex, char_digit[digit]);
    ex -= CHAR_WIDTH + CHAR_SPACE;
  }
  // print space
  ex -= CHAR_WIDTH + CHAR_SPACE;
  // print DD
  for (int i = 0; i < 2; ++i) {
    int digit = time_info.day_str[9 - i] - '0';
    graph_draw_char(graph_img, ey, ex, char_digit[digit]);
    ex -= CHAR_WIDTH + CHAR_SPACE;
  }
  // print /
  graph_draw_char(graph_img, ey, ex, char_slash);
  ex -= CHAR_WIDTH + CHAR_SPACE;
  // print MM
  for (int i = 0; i < 2; ++i) {
    int digit = time_info.day_str[6 - i] - '0';
    graph_draw_char(graph_img, ey, ex, char_digit[digit]);
    ex -= CHAR_WIDTH + CHAR_SPACE;
  }
  // print /
  graph_draw_char(graph_img, ey, ex, char_slash);
  ex -= CHAR_WIDTH + CHAR_SPACE;
  // print YYYY
  for (int i = 0; i < 4; ++i) {
    int digit = time_info.day_str[3 - i] - '0';
    graph_draw_char(graph_img, ey, ex, char_digit[digit]);
    ex -= CHAR_WIDTH + CHAR_SPACE;
  }
}


void graph_draw_title(Graph_img &graph_img, const int str[], int len_str) {
  int ex = (float)GRAPH_IMG_WIDTH / 2.0 + (float)len_str / 2.0 * CHAR_WIDTH + (float)(len_str - 1) / 2.0 * CHAR_SPACE;
  int ey = GRAPH_IMG_HEIGHT - CHAR_HEIGHT - CHAR_TITLE_MARGIN_Y;
  for (int i = len_str - 1; i >= 0; --i) {
    graph_draw_char(graph_img, ey, ex, char_list[str[i]]);
    ex -= CHAR_WIDTH + CHAR_SPACE;
  }
}


void graph_draw_x_scale(Graph_img &graph_img, Graph_data &graph_data) {
  // x scale + daytime-night coloring
  for (int32_t x = 0; x < GRAPH_DATA_N; ++x){
    int32_t minute = graph_data.last_data_update_minute + 1440 - (GRAPH_DATA_N - 1 - x) * GRAPH_DATA_INTERVAL;
    int32_t hour = graph_data.last_data_update_hour + minute / 60;
    if (6 <= hour % 24 && hour % 24 < 18){ // daytime
      for (int y = 0; y <= GRAPH_AREA_HEIGHT; ++y){
        graph_img.graph[GRAPH_SY + y][GRAPH_SX + x] = PALETTE_YELLOW;
      }
    } else{ // night
      for (int y = 0; y <= GRAPH_AREA_HEIGHT; ++y){
        graph_img.graph[GRAPH_SY + y][GRAPH_SX + x] = PALETTE_LIGHTBLUE;
      }
    }
    if (minute % 60 == 0){ // every 1 hour
      graph_img.graph[GRAPH_SY - 2][GRAPH_SX + x] = PALETTE_GRAY;
      if (hour % 3 == 0){ // every 3 hour
        for (int y = 0; y <= GRAPH_AREA_HEIGHT; ++y){
          graph_img.graph[GRAPH_SY + y][GRAPH_SX + x] = PALETTE_LIGHTGRAY;
        }
      }
      if (hour % 12 == 0){ // 0 o'clock & 12 o'clock
        graph_img.graph[GRAPH_SY - 3][GRAPH_SX + x] = PALETTE_GRAY;
        graph_img.graph[GRAPH_SY - 4][GRAPH_SX + x] = PALETTE_GRAY;
      }
      if (hour % 24 == 0){ // 0 o'clock
        graph_img.graph[GRAPH_SY - 5][GRAPH_SX + x] = PALETTE_GRAY;
      }
    }
  }
}


void graph_draw_y_scale(Graph_img &graph_img, int y_min, int y_max, const Value_color scale[], int n_scale) {
  for (int i = 0; i < n_scale; ++i) {
    if (y_min <= scale[i].value && scale[i].value <= y_max) {
      int32_t ey = GRAPH_SY + round((float)GRAPH_AREA_HEIGHT * (scale[i].value - y_min) / (y_max - y_min)) - CHAR_HEIGHT / 2;
      int32_t ex = GRAPH_SX - 4;
      int v = scale[i].value;
      while (v) {
        int digit = v % 10;
        graph_draw_char(graph_img, ey, ex, char_digit[digit]);
        ex -= CHAR_WIDTH + CHAR_SPACE;
        v /= 10;
      }
    }
  }
}


void graph_draw_frame(Graph_img &graph_img) {
  // y = y_min
  for (int32_t x = 0; x <= GRAPH_AREA_WIDTH; ++x){
    graph_img.graph[GRAPH_SY - 1][GRAPH_SX + x] = PALETTE_GRAY;
  }
  
  // y = y_max
  for (int32_t x = 0; x <= GRAPH_AREA_WIDTH; ++x){
    graph_img.graph[GRAPH_SY + GRAPH_AREA_HEIGHT][GRAPH_SX + x] = PALETTE_GRAY;
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


void graph_draw_graph_line(Graph_img &graph_img, int fy, int fx, int ny, int nx) {
  int min_y = min(fy, ny);
  int max_y = max(fy, ny);
  for (int y = min_y + 1; y < max_y; ++y) {
    int x = round((float)fx + (float)(y - fy) / (ny - fy) * (nx - fx));
    graph_img.graph[GRAPH_SY + y][GRAPH_SX + x] = PALETTE_BLACK;
  }
  /*
  if (fy < ny){
    for (int32_t y = fy + 1; y < y_mid; ++y){
      graph_img.graph[GRAPH_SY + y][GRAPH_SX + x - 1] = PALETTE_BLACK;
    }
    for (int32_t y = y_mid; y < ny; ++y){
      graph_img.graph[GRAPH_SY + y][GRAPH_SX + x] = PALETTE_BLACK;
    }
  } else if (fy > ny){
    for (int32_t y = fy - 1; y > y_mid; --y){
      graph_img.graph[GRAPH_SY + y][GRAPH_SX + x - 1] = PALETTE_BLACK;
    }
    for (int32_t y = y_mid; y > ny; --y){
      graph_img.graph[GRAPH_SY + y][GRAPH_SX + x] = PALETTE_BLACK;
    }
  }
  */
}



void graph_draw_temperature(Graph_data &graph_data, Graph_img &graph_img, Time_info &time_info){
  graph_draw_white(graph_img);
  graph_draw_x_scale(graph_img, graph_data);

  // y range
  int y_min = 10000, y_max = -10000;
  for (int i = 0; i < GRAPH_DATA_N; ++i){
    if (graph_data.temperature[i] != GRAPH_DATA_UNDEFINED){
      y_min = min(y_min, (int)graph_data.temperature[i] - 1);
      y_max = max(y_max, (int)graph_data.temperature[i] + 1);
    }
  }
  if (y_min == 10000){
    y_min = 20;
  }
  if (y_max == -10000){
    y_max = 20;
  }

  while (true) {
    int n_main_scale = 0;
    for (int32_t t = y_min; t <= y_max; ++t){
      for (int i = 0; i < N_COLOR_TEMPERATURE; ++i){
        if (t == color_temperature[i].value){
          ++n_main_scale;
        }
      }
    }
    if (n_main_scale >= 2) {
      break;
    }
    int n_sub_min = (y_min + 100) % 5;
    if (n_sub_min == 0) {
      n_sub_min = 5;
    }
    if (y_min <= color_temperature[0].value) {
      n_sub_min = 100;
    }
    int n_add_max = 5 - (y_max + 100) % 5;
    if (y_max >= color_temperature[N_COLOR_TEMPERATURE - 1].value) {
      n_add_max = 100;
    }
    if (n_sub_min < n_add_max) {
      y_min -= n_sub_min + 1;
    } else{
      y_max += n_add_max + 1;
    }
  }

  // y scale
  for (int32_t t = y_min; t <= y_max; ++t){
    uint8_t line_color = PALETTE_LIGHTGRAY;
    for (int i = 0; i < N_COLOR_TEMPERATURE; ++i){
      if (t == color_temperature[i].value){
        line_color = color_temperature[i].color;
      }
    }
    int32_t y = round((float)GRAPH_AREA_HEIGHT * (t - y_min) / (y_max - y_min));
    for (int32_t x = 0; x <= GRAPH_AREA_WIDTH; ++x){
      graph_img.graph[GRAPH_SY + y][GRAPH_SX + x] = line_color;
    }
  }

  // plot temperature graph
  int32_t fy = GRAPH_DATA_UNDEFINED;
  for (int32_t x = 0; x < GRAPH_DATA_N; ++x){
    if (graph_data.temperature[x] != GRAPH_DATA_UNDEFINED){
      int32_t y = round((float)GRAPH_AREA_HEIGHT * (graph_data.temperature[x] - y_min) / (y_max - y_min));
      graph_img.graph[GRAPH_SY + y][GRAPH_SX + x] = PALETTE_BLACK;
      if (fy != GRAPH_DATA_UNDEFINED){
        graph_draw_graph_line(graph_img, fy, x - 1, y, x);
      }
      fy = y;
    }
  }

  graph_draw_y_scale(graph_img, y_min, y_max, color_temperature, N_COLOR_TEMPERATURE);
  graph_draw_frame(graph_img);
  graph_draw_title(graph_img, char_idx_temperature, CHAR_TEMPERATURE_N);
  graph_draw_time(graph_img, time_info);
}



void graph_draw_humidity(Graph_data &graph_data, Graph_img &graph_img, Time_info &time_info){
  graph_draw_white(graph_img);
  graph_draw_x_scale(graph_img, graph_data);

  // y scale
  for (int32_t t = GRAPH_HUMIDITY_Y_MIN; t <= GRAPH_HUMIDITY_Y_MAX; t += GRAPH_HUMIDITY_SCALE_INTERVAL){
    uint8_t line_color = PALETTE_LIGHTGRAY;
    for (int i = 0; i < N_COLOR_HUMIDITY; ++i){
      if (t == color_humidity[i].value){
        line_color = color_humidity[i].color;
      }
    }
    int32_t y = round((float)GRAPH_AREA_HEIGHT * (t - GRAPH_HUMIDITY_Y_MIN) / (GRAPH_HUMIDITY_Y_MAX - GRAPH_HUMIDITY_Y_MIN));
    for (int32_t x = 0; x <= GRAPH_AREA_WIDTH; ++x){
      graph_img.graph[GRAPH_SY + y][GRAPH_SX + x] = line_color;
    }
  }

  // plot humidity graph
  int32_t fy = GRAPH_DATA_UNDEFINED;
  for (int32_t x = 0; x < GRAPH_DATA_N; ++x){
    if (graph_data.humidity[x] != GRAPH_DATA_UNDEFINED){
      int32_t y = round((float)GRAPH_AREA_HEIGHT * (graph_data.humidity[x] - GRAPH_HUMIDITY_Y_MIN) / (GRAPH_HUMIDITY_Y_MAX - GRAPH_HUMIDITY_Y_MIN));
      graph_img.graph[GRAPH_SY + y][GRAPH_SX + x] = PALETTE_BLACK;
      if (fy != GRAPH_DATA_UNDEFINED){
        graph_draw_graph_line(graph_img, fy, x - 1, y, x);
      }
      fy = y;
    }
  }

  graph_draw_y_scale(graph_img, GRAPH_HUMIDITY_Y_MIN, GRAPH_HUMIDITY_Y_MAX, color_humidity, N_COLOR_HUMIDITY);
  graph_draw_frame(graph_img);
  graph_draw_title(graph_img, char_idx_humidity, CHAR_HUMIDITY_N);
  graph_draw_time(graph_img, time_info);
}




void graph_draw_pressure(Graph_data &graph_data, Graph_img &graph_img, Time_info &time_info){
  graph_draw_white(graph_img);
  graph_draw_x_scale(graph_img, graph_data);

  // y range
  int y_min = 975, y_max = 1025;
  for (int i = 0; i < GRAPH_DATA_N; ++i){
    if (graph_data.pressure[i] != GRAPH_DATA_UNDEFINED){
      y_min = min(y_min, (int)graph_data.pressure[i] - 1);
      y_max = max(y_max, (int)graph_data.pressure[i] + 1);
    }
  }
  y_min -= y_min % GRAPH_PRESSURE_SCALE_INTERVAL;
  y_max += (GRAPH_PRESSURE_SCALE_INTERVAL - y_max % GRAPH_PRESSURE_SCALE_INTERVAL) % GRAPH_PRESSURE_SCALE_INTERVAL;

  // y scale
  for (int32_t t = y_min; t <= y_max; t += GRAPH_PRESSURE_SCALE_INTERVAL){
    uint8_t line_color = PALETTE_LIGHTGRAY;
    for (int i = 0; i < N_COLOR_PRESSURE; ++i){
      if (t == color_pressure[i].value){
        line_color = color_pressure[i].color;
      }
    }
    int32_t y = round((float)GRAPH_AREA_HEIGHT * (t - y_min) / (y_max - y_min));
    for (int32_t x = 0; x <= GRAPH_AREA_WIDTH; ++x){
      graph_img.graph[GRAPH_SY + y][GRAPH_SX + x] = line_color;
    }
  }

  // plot pressure graph
  int32_t fy = GRAPH_DATA_UNDEFINED;
  for (int32_t x = 0; x < GRAPH_DATA_N; ++x){
    if (graph_data.pressure[x] != GRAPH_DATA_UNDEFINED){
      int32_t y = round((float)GRAPH_AREA_HEIGHT * (graph_data.pressure[x] - y_min) / (y_max - y_min));
      graph_img.graph[GRAPH_SY + y][GRAPH_SX + x] = PALETTE_BLACK;
      if (fy != GRAPH_DATA_UNDEFINED){
        graph_draw_graph_line(graph_img, fy, x - 1, y, x);
      }
      fy = y;
    }
  }

  graph_draw_y_scale(graph_img, y_min, y_max, color_pressure, N_COLOR_PRESSURE);
  graph_draw_frame(graph_img);
  graph_draw_title(graph_img, char_idx_pressure, CHAR_PRESSURE_N);
  graph_draw_time(graph_img, time_info);
}



void graph_draw_co2_concentration(Graph_data &graph_data, Graph_img &graph_img, Time_info &time_info){
  graph_draw_white(graph_img);
  graph_draw_x_scale(graph_img, graph_data);

  // y range
  int y_min = 400, y_max = 1100;
  for (int i = 0; i < GRAPH_DATA_N; ++i){
    if (graph_data.co2_concentration[i] != GRAPH_DATA_UNDEFINED){
      y_min = min(y_min, (int)graph_data.co2_concentration[i] - 1);
      y_max = max(y_max, (int)graph_data.co2_concentration[i] + 1);
    }
  }
  y_min -= y_min % GRAPH_CO2_CONCENTRATION_SCALE_INTERVAL;
  y_max += (GRAPH_CO2_CONCENTRATION_SCALE_INTERVAL - y_max % GRAPH_CO2_CONCENTRATION_SCALE_INTERVAL) % GRAPH_CO2_CONCENTRATION_SCALE_INTERVAL;

  // y scale
  for (int32_t t = y_min; t <= y_max; t += GRAPH_CO2_CONCENTRATION_SCALE_INTERVAL){
    uint8_t line_color = PALETTE_LIGHTGRAY;
    for (int i = 0; i < N_COLOR_CO2_CONCENTRATION; ++i){
      if (t == color_co2_concentration[i].value){
        line_color = color_co2_concentration[i].color;
      }
    }
    int32_t y = round((float)GRAPH_AREA_HEIGHT * (t - y_min) / (y_max - y_min));
    for (int32_t x = 0; x <= GRAPH_AREA_WIDTH; ++x){
      graph_img.graph[GRAPH_SY + y][GRAPH_SX + x] = line_color;
    }
  }

  // plot co2 concentration graph
  int32_t fy = GRAPH_DATA_UNDEFINED;
  for (int32_t x = 0; x < GRAPH_DATA_N; ++x){
    if (graph_data.co2_concentration[x] != GRAPH_DATA_UNDEFINED){
      int32_t y = round((float)GRAPH_AREA_HEIGHT * (graph_data.co2_concentration[x] - y_min) / (y_max - y_min));
      graph_img.graph[GRAPH_SY + y][GRAPH_SX + x] = PALETTE_BLACK;
      if (fy != GRAPH_DATA_UNDEFINED){
        graph_draw_graph_line(graph_img, fy, x - 1, y, x);
      }
      fy = y;
    }
  }

  graph_draw_y_scale(graph_img, y_min, y_max, color_co2_concentration, N_COLOR_CO2_CONCENTRATION);
  graph_draw_frame(graph_img);
  graph_draw_title(graph_img, char_idx_co2_concentration, CHAR_CO2_CONCENTRATION_N);
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
  }
  graph_data.temperature[GRAPH_DATA_N - 1] = sensor_data.temperature;
  graph_data.humidity[GRAPH_DATA_N - 1] = sensor_data.humidity;
  graph_data.pressure[GRAPH_DATA_N - 1] = sensor_data.pressure;
  graph_data.co2_concentration[GRAPH_DATA_N - 1] = sensor_data.co2_concentration;
}



void graph_data_update(Time_info &time_info, Graph_data &graph_data, Sensor_data &sensor_data){
  if (time_info.minute != graph_data.last_data_update_minute && time_info.minute % GRAPH_DATA_INTERVAL == 0){
    graph_data.last_data_update_minute = time_info.minute;
    graph_data.last_data_update_hour = time_info.hour;
    graph_add_data(graph_data, sensor_data);
  }
}














