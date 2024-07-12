#include "graph.h"

const uint8_t color_palette[BMP_N_COLOR_PALETTE][3] = { // RGB
  {255, 255, 255}, // white
  {  0,   0,   0}, // black
  {255,   0,   0}, // red
  { 80, 230,  78}, // green
  { 54, 194, 206}, // sky blue
  {  0,   0, 255}, // blue
  {255, 178,  44}, // orange
  {250, 255, 175}, // yellow
  {187, 233, 255}, // light blue
  {127, 127, 127}, // gray
  {200, 200, 200} // light gray
};

const Value_color color_temperature[N_COLOR_TEMPERATURE] = {
  {10, PALETTE_BLUE},
  {15, PALETTE_SKYBLUE},
  {20, PALETTE_GREEN},
  {25, PALETTE_ORANGE},
  {30, PALETTE_RED}
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

void graph_draw(Graph_data &graph_data, Graph_img &graph_img){
  // white canvas
  for (int y = 0; y < GRAPH_IMG_HEIGHT; ++y){
    for (int x = 0; x < GRAPH_IMG_WIDTH; ++x){
      graph_img.graph[y][x] = PALETTE_WHITE;
   }
  }

  // y range
  int y_min = 10000, y_max = -10000;
  for (int i = 0; i < GRAPH_DATA_N; ++i){
    if (graph_data.temperature[i] != GRAPH_DATA_UNDEFINED){
      y_min = min(y_min, (int)graph_data.temperature[i] - 1);
      y_max = max(y_max, (int)graph_data.temperature[i] + 1);
    }
  }
  if (y_min == 10000){
    y_min = 0;
  }
  if (y_max == -10000){
    y_max = 0;
  }
  if (y_max - y_min < 6){
    y_min -= 3;
    y_max += 3;
  }

  // x scale + daytime-night coloring
  for (int32_t x = 0; x < GRAPH_DATA_N; ++x){
    int32_t minute = graph_data.last_data_update_minute + 1440 - (GRAPH_DATA_N - 1 - x) * GRAPH_DATA_INTERVAL;
    int32_t hour = graph_data.last_data_update_hour + minute / 60;
    Serial.print(graph_data.last_data_update_hour);
    Serial.print(':');
    Serial.print(graph_data.last_data_update_minute);
    Serial.print('\t');
    Serial.print(x);
    Serial.print('\t');
    Serial.print(hour);
    Serial.print(' ');
    Serial.print(hour % 24);
    Serial.print('\t');
    Serial.print(minute);
    Serial.print(' ');
    Serial.println(minute % 60);
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
      graph_img.graph[GRAPH_SY - 1][GRAPH_SX + x] = PALETTE_GRAY;
      if (hour % 3 == 0){ // every 3 hour
        for (int y = 0; y <= GRAPH_AREA_HEIGHT; ++y){
          graph_img.graph[GRAPH_SY + y][GRAPH_SX + x] = PALETTE_LIGHTGRAY;
        }
        if (hour % 12 == 0){ // 0 o'clock & 12 o'clock
          graph_img.graph[GRAPH_SY - 2][GRAPH_SX + x] = PALETTE_GRAY;
          graph_img.graph[GRAPH_SY - 3][GRAPH_SX + x] = PALETTE_GRAY;
          if (hour % 24 == 0){ // 0 o'clock
            graph_img.graph[GRAPH_SY - 4][GRAPH_SX + x] = PALETTE_GRAY;
            graph_img.graph[GRAPH_SY - 5][GRAPH_SX + x] = PALETTE_GRAY;
          }
        }
      }
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
    int32_t y = (float)GRAPH_AREA_HEIGHT * (t - y_min) / (y_max - y_min);
    for (int32_t x = 0; x <= GRAPH_AREA_WIDTH; ++x){
      graph_img.graph[GRAPH_SY + y][GRAPH_SX + x] = line_color;
    }
  }

  // y = y_min
  for (int32_t x = 0; x <= GRAPH_AREA_WIDTH; ++x){
    graph_img.graph[GRAPH_SY][GRAPH_SX + x] = PALETTE_GRAY;
  }
  
  // y = y_max
  for (int32_t x = 0; x <= GRAPH_AREA_WIDTH; ++x){
    graph_img.graph[GRAPH_SY + GRAPH_AREA_HEIGHT][GRAPH_SX + x] = PALETTE_GRAY;
  }

  // x = 0
  for (int32_t y = 0; y <= GRAPH_AREA_HEIGHT; ++y){
    graph_img.graph[GRAPH_SY + y][GRAPH_SX - 1] = PALETTE_GRAY;
  }

  // x = end
  for (int32_t y = 0; y <= GRAPH_AREA_HEIGHT; ++y){
    graph_img.graph[GRAPH_SY + y][GRAPH_SX + GRAPH_AREA_WIDTH] = PALETTE_GRAY;
  }

  // plot temperature graph
  int32_t fy = GRAPH_DATA_UNDEFINED;
  for (int32_t x = 0; x < GRAPH_DATA_N; ++x){
    if (graph_data.temperature[x] != GRAPH_DATA_UNDEFINED){
      int32_t y = (float)GRAPH_AREA_HEIGHT * (graph_data.temperature[x] - y_min) / (y_max - y_min);
      graph_img.graph[GRAPH_SY + y][GRAPH_SX + x] = PALETTE_BLACK;
      if (fy != GRAPH_DATA_UNDEFINED){
        int32_t y_mid = (fy + y) / 2;
        if (fy < y){
          for (int32_t yy = fy + 1; yy < y_mid; ++yy){
            graph_img.graph[GRAPH_SY + yy][GRAPH_SX + x - 1] = PALETTE_BLACK;
          }
          for (int32_t yy = y_mid; yy < y; ++yy){
            graph_img.graph[GRAPH_SY + yy][GRAPH_SX + x] = PALETTE_BLACK;
          }
        } else if (fy > y){
          for (int32_t yy = fy - 1; yy > y_mid; --yy){
            graph_img.graph[GRAPH_SY + yy][GRAPH_SX + x - 1] = PALETTE_BLACK;
          }
          for (int32_t yy = y_mid; yy > y; --yy){
            graph_img.graph[GRAPH_SY + yy][GRAPH_SX + x] = PALETTE_BLACK;
          }
        }
      }
      fy = y;
    }
  }
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
  }
  graph_data.temperature[GRAPH_DATA_N - 1] = sensor_data.temperature;
}



void graph_data_update(Time_info &time_info, Graph_data &graph_data, Sensor_data &sensor_data){
  if (time_info.minute != graph_data.last_data_update_minute && time_info.minute % GRAPH_DATA_INTERVAL == 0){
    graph_data.last_data_update_minute = time_info.minute;
    graph_data.last_data_update_hour = time_info.hour;
    graph_add_data(graph_data, sensor_data);
  }
}














