#ifndef GRAPH_H
#define GRAPH_H

// graph data
#define GRAPH_DATA_INTERVAL 10 // minute
#define GRAPH_DATA_N 144 // 1 day
#define GRAPH_DATA_UNDEFINED -9999.0
struct Graph_data{
  float temperature[GRAPH_DATA_N];
  int last_data_update_minute;

  Graph_data(){
    for (int i = 0; i < GRAPH_DATA_N; ++i){
      temperature[i] = GRAPH_DATA_UNDEFINED;
    }
    last_data_update_minute = -1;
  }
};



// bitmap color palette
#define BMP_N_COLOR_PALETTE 9
#define PALETTE_WHITE 0
#define PALETTE_BLACK 1
#define PALETTE_RED 2
#define PALETTE_GREEN 3
#define PALETTE_SKYBLUE 4
#define PALETTE_BLUE 5
#define PALETTE_ORANGE 6
#define PALETTE_GRAY 7
#define PALETTE_LIGHTGRAY 8



// graph area
#define GRAPH_IMG_HEIGHT 140
#define GRAPH_IMG_WIDTH 160
#define GRAPH_AREA_HEIGHT 120
#define GRAPH_AREA_WIDTH 144
#define GRAPH_SX 8
#define GRAPH_SY 10



// graph bitmap image
#define BMP_BIT_PER_PIXEL 4
#define BMP_HEADER_BYTE (14 + 40)
#define BMP_OFFSET_TO_IMG_DATA (BMP_HEADER_BYTE + BMP_N_COLOR_PALETTE * 4)
#define BMP_GRAPH_FILE_SIZE (BMP_OFFSET_TO_IMG_DATA + GRAPH_IMG_HEIGHT * GRAPH_IMG_WIDTH * BMP_BIT_PER_PIXEL / 8)
#define BMP_GRAPH_FILE_NAME "graph.bmp"
#define HTTP_BOUNDARY "boundary"
struct Slack_bmp_const{
  String bmp_https_head, bmp_https_foot;
  int32_t len_bmp_https_head, len_bmp_https_foot;
  
  Slack_bmp_const(){
    // https header
    bmp_https_head = "";
    bmp_https_head += String("--") + HTTP_BOUNDARY + "\r\n";
    bmp_https_head += String("Content-Disposition: form-data; name=\"uploadFile\"; filename=\"./") + BMP_GRAPH_FILE_NAME + "\"\r\n";
    bmp_https_head += "\r\n";
    len_bmp_https_head = bmp_https_head.length();

    // https footer
    bmp_https_foot = "";
    bmp_https_foot += "\r\n";
    bmp_https_foot += String("--") + HTTP_BOUNDARY + "--\r\n";
    bmp_https_foot += "\r\n";
    len_bmp_https_foot = bmp_https_foot.length();
  }
};



// graph temperature color
#define N_COLOR_TEMPERATURE 5
struct Value_color{
  int value;
  uint8_t color;
};



// graph bitmap
struct Graph_img{
  uint8_t graph[GRAPH_IMG_HEIGHT][GRAPH_IMG_WIDTH];
  uint8_t bmp_img[BMP_GRAPH_FILE_SIZE];
};




#endif