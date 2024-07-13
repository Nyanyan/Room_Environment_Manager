#ifndef GRAPH_H
#define GRAPH_H

// graph data
#define GRAPH_DATA_INTERVAL 10 // minute
#define GRAPH_DATA_N 144 // 1 day
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
#define BMP_N_COLOR_PALETTE 13
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



// graph area
#define GRAPH_IMG_HEIGHT 146
#define GRAPH_IMG_WIDTH 184
#define GRAPH_AREA_HEIGHT 120
#define GRAPH_AREA_WIDTH 144
#define GRAPH_SX 32
#define GRAPH_SY 14



// graph bitmap image
#define BMP_BIT_PER_PIXEL 4
#define BMP_HEADER_BYTE (14 + 40)
#define BMP_OFFSET_TO_IMG_DATA (BMP_HEADER_BYTE + BMP_N_COLOR_PALETTE * 4)
#define BMP_GRAPH_FILE_SIZE (BMP_OFFSET_TO_IMG_DATA + GRAPH_IMG_HEIGHT * GRAPH_IMG_WIDTH * BMP_BIT_PER_PIXEL / 8)
#define BMP_GRAPH_FILE_NAME_TEMPERATURE "temperature.bmp"
#define BMP_GRAPH_FILE_NAME_HUMIDITY "humidity.bmp"
#define BMP_GRAPH_FILE_NAME_PRESSURE "pressure.bmp"
#define BMP_GRAPH_FILE_NAME_CO2_CONCENTRATION "co2.bmp"
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
#define N_COLOR_HUMIDITY 4
#define GRAPH_HUMIDITY_SCALE_INTERVAL 10
#define GRAPH_HUMIDITY_Y_MIN 0
#define GRAPH_HUMIDITY_Y_MAX 100

// pressure graph
#define N_COLOR_PRESSURE 5
#define GRAPH_PRESSURE_SCALE_INTERVAL 5

// co2 concentration graph
#define N_COLOR_CO2_CONCENTRATION 6
#define GRAPH_CO2_CONCENTRATION_SCALE_INTERVAL 50


// graph bitmap
struct Graph_img{
  uint8_t graph[GRAPH_IMG_HEIGHT][GRAPH_IMG_WIDTH];
  uint8_t bmp_img[BMP_GRAPH_FILE_SIZE];
};




#endif