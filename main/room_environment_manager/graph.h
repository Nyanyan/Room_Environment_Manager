#ifndef GRAPH_H
#define GRAPH_H

#include <JPEGENC.h>

// graph data
#define GRAPH_DATA_INTERVAL 10 // minute
#define GRAPH_DATA_1_DAY 144 // 1 day
#define GRAPH_DATA_N (GRAPH_DATA_1_DAY * 2) // 1 day * 2
#define GRAPH_DATA_UNDEFINED -9999.0
struct Graph_data{
  float temperature[GRAPH_DATA_N];
  float humidity[GRAPH_DATA_N];
  float pressure[GRAPH_DATA_N];
  float co2_concentration[GRAPH_DATA_N];
  int last_data_update_minute;
  int last_data_update_hour;

  Graph_data(){
    for (int i = 0; i < GRAPH_DATA_N; ++i){
      temperature[i] = GRAPH_DATA_UNDEFINED;
      humidity[i] = GRAPH_DATA_UNDEFINED;
      pressure[i] = GRAPH_DATA_UNDEFINED;
      co2_concentration[i] = GRAPH_DATA_UNDEFINED;
    }
    last_data_update_minute = -1;
    last_data_update_hour = -1;
  }
};



// bitmap color palette
#define BMP_N_COLOR_PALETTE 14
#define PALETTE_WHITE 0
#define PALETTE_BLACK 1
#define PALETTE_GRAY 2
#define PALETTE_LIGHTGRAY 3
#define PALETTE_RED 4
#define PALETTE_GREEN 5
#define PALETTE_SKYBLUE 6
#define PALETTE_BLUE 7
#define PALETTE_ORANGE 8
#define PALETTE_YELLOW 9
#define PALETTE_LIGHTBLUE 10
#define PALETTE_PURPLE 11
#define PALETTE_NAVYBLUE 12
#define PALETTE_DARKBLUE 13



// graph area
#define GRAPH_IMG_HEIGHT 146
#define GRAPH_IMG_WIDTH 184
#define GRAPH_AREA_HEIGHT 120
#define GRAPH_AREA_WIDTH 144
#define GRAPH_SX 32
#define GRAPH_SY 13



// JPEG output buffer
#define JPEG_GRAPH_BUF_SIZE 30720
#define JPEG_GRAPH_FILE_NAME_TEMPERATURE "temperature.jpg"
#define JPEG_GRAPH_FILE_NAME_HUMIDITY "humidity.jpg"
#define JPEG_GRAPH_FILE_NAME_PRESSURE "pressure.jpg"
#define JPEG_GRAPH_FILE_NAME_CO2_CONCENTRATION "co2.jpg"
#define HTTP_BOUNDARY "boundary"


// graph color scale
struct Value_color{
  int value;
  uint8_t color;
};


// temperature graph
#define N_COLOR_TEMPERATURE 7
#define GRAPH_TEMPERATURE_SCALE_INTERVAL 1

// humidity graph
#define N_COLOR_HUMIDITY 6
#define GRAPH_HUMIDITY_SCALE_INTERVAL 10

// pressure graph
#define N_COLOR_PRESSURE 4
#define GRAPH_PRESSURE_SCALE_INTERVAL 5

// co2 concentration graph
#define N_COLOR_CO2_CONCENTRATION 7
#define GRAPH_CO2_CONCENTRATION_SCALE_INTERVAL 50


// graph image
struct Graph_img{
  uint8_t graph[GRAPH_IMG_HEIGHT][GRAPH_IMG_WIDTH];
  uint8_t jpeg_buf[JPEG_GRAPH_BUF_SIZE];
  uint32_t jpeg_size;
};




#endif