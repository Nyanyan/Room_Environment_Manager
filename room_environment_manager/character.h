#ifndef CHARACTER_H
#define CHARACTER_H

// character
#define CHAR_WIDTH 5
#define CHAR_HEIGHT 7
#define CHAR_SPACE 1

// time information
#define CHAR_TIME_MARGIN_X 4
#define CHAR_TIME_MARGIN_Y 2

// graph title
#define CHAR_TITLE_MARGIN_Y 2
#define CHAR_TEMPERATURE_N 11
#define CHAR_HUMIDITY_N 8
#define CHAR_PRESSURE_N 8
#define CHAR_CO2_CONCENTRATION_N 3

// graph unit
#define CHAR_UNIT_MARGIN_Y 2
#define CHAR_UNIT_EX (GRAPH_SX - 4)
#define CHAR_DEGREE_N 1 // *C (special character)
#define CHAR_PERCENT_N 1 // %
#define CHAR_HPA_N 3 // hPa
#define CHAR_PPM_N 3 // ppm

// alphabet
#define CHAR_N_LIST 22
#define CHAR_LIST_a 0
#define CHAR_LIST_d 1
#define CHAR_LIST_e 2
#define CHAR_LIST_h 3
#define CHAR_LIST_i 4
#define CHAR_LIST_m 5
#define CHAR_LIST_p 6
#define CHAR_LIST_r 7
#define CHAR_LIST_s 8
#define CHAR_LIST_t 9
#define CHAR_LIST_u 10
#define CHAR_LIST_y 11
#define CHAR_LIST_C 12
#define CHAR_LIST_H 13
#define CHAR_LIST_O 14
#define CHAR_LIST_P 15
#define CHAR_LIST_T 16
#define CHAR_LIST_2 17
#define CHAR_LIST_SLASH 18
#define CHAR_LIST_COLON 19
#define CHAR_LIST_DEGREE 20
#define CHAR_LIST_PERCENT 21

const int char_idx_temperature[CHAR_TEMPERATURE_N] = {
  CHAR_LIST_T, 
  CHAR_LIST_e, 
  CHAR_LIST_m, 
  CHAR_LIST_p, 
  CHAR_LIST_e, 
  CHAR_LIST_r, 
  CHAR_LIST_a, 
  CHAR_LIST_t, 
  CHAR_LIST_u, 
  CHAR_LIST_r, 
  CHAR_LIST_e
};

const int char_idx_humidity[CHAR_HUMIDITY_N] = {
  CHAR_LIST_H, 
  CHAR_LIST_u, 
  CHAR_LIST_m, 
  CHAR_LIST_i, 
  CHAR_LIST_d, 
  CHAR_LIST_i, 
  CHAR_LIST_t, 
  CHAR_LIST_y
};

const int char_idx_pressure[CHAR_PRESSURE_N] = {
  CHAR_LIST_P, 
  CHAR_LIST_r, 
  CHAR_LIST_e, 
  CHAR_LIST_s, 
  CHAR_LIST_s, 
  CHAR_LIST_u, 
  CHAR_LIST_r, 
  CHAR_LIST_e
};

const int char_idx_co2_concentration[CHAR_CO2_CONCENTRATION_N] = {
  CHAR_LIST_C, 
  CHAR_LIST_O, 
  CHAR_LIST_2
};

const int char_idx_degree[CHAR_DEGREE_N] = {
  CHAR_LIST_DEGREE
};

const int char_idx_percent[CHAR_PERCENT_N] = {
  CHAR_LIST_PERCENT
};

const int char_idx_hpa[CHAR_HPA_N] = {
  CHAR_LIST_h, 
  CHAR_LIST_P, 
  CHAR_LIST_a
};

const int char_idx_ppm[CHAR_PPM_N] = {
  CHAR_LIST_p, 
  CHAR_LIST_p, 
  CHAR_LIST_m
};



const bool char_digit[10][CHAR_HEIGHT][CHAR_WIDTH] = {
  { // 0
    {0, 1, 1, 1, 0}, 
    {1, 0, 0, 0, 1}, 
    {1, 0, 0, 0, 1}, 
    {1, 0, 0, 0, 1}, 
    {1, 0, 0, 0, 1}, 
    {1, 0, 0, 0, 1}, 
    {0, 1, 1, 1, 0}
  },
  { // 1
    {0, 0, 1, 0, 0}, 
    {0, 1, 1, 0, 0}, 
    {0, 0, 1, 0, 0}, 
    {0, 0, 1, 0, 0}, 
    {0, 0, 1, 0, 0}, 
    {0, 0, 1, 0, 0}, 
    {0, 1, 1, 1, 0}
  },
  { // 2
    {0, 1, 1, 1, 0}, 
    {1, 0, 0, 0, 1}, 
    {0, 0, 0, 0, 1}, 
    {0, 0, 0, 1, 0}, 
    {0, 0, 1, 0, 0}, 
    {0, 1, 0, 0, 0}, 
    {1, 1, 1, 1, 1}
  },
  { // 3
    {1, 1, 1, 1, 1}, 
    {0, 0, 0, 1, 0}, 
    {0, 0, 1, 0, 0}, 
    {0, 0, 0, 1, 0}, 
    {0, 0, 0, 0, 1}, 
    {1, 0, 0, 0, 1}, 
    {0, 1, 1, 1, 0}
  },
  { // 4
    {0, 0, 0, 1, 0}, 
    {0, 0, 1, 1, 0}, 
    {0, 1, 0, 1, 0}, 
    {1, 0, 0, 1, 0}, 
    {1, 1, 1, 1, 1}, 
    {0, 0, 0, 1, 0}, 
    {0, 0, 0, 1, 0}
  },
  { // 5
    {1, 1, 1, 1, 1}, 
    {1, 0, 0, 0, 0}, 
    {1, 1, 1, 1, 0}, 
    {0, 0, 0, 0, 1}, 
    {0, 0, 0, 0, 1}, 
    {1, 0, 0, 0, 1}, 
    {0, 1, 1, 1, 0}
  },
  { // 6
    {0, 0, 1, 1, 0}, 
    {0, 1, 0, 0, 0}, 
    {1, 0, 0, 0, 0}, 
    {1, 1, 1, 1, 0}, 
    {1, 0, 0, 0, 1}, 
    {1, 0, 0, 0, 1}, 
    {0, 1, 1, 1, 0}
  },
  { // 7
    {1, 1, 1, 1, 1}, 
    {1, 0, 0, 0, 1}, 
    {0, 0, 0, 1, 0}, 
    {0, 0, 1, 0, 0}, 
    {0, 1, 0, 0, 0}, 
    {0, 1, 0, 0, 0}, 
    {0, 1, 0, 0, 0}
  },
  { // 8
    {0, 1, 1, 1, 0}, 
    {1, 0, 0, 0, 1}, 
    {1, 0, 0, 0, 1}, 
    {0, 1, 1, 1, 0}, 
    {1, 0, 0, 0, 1}, 
    {1, 0, 0, 0, 1}, 
    {0, 1, 1, 1, 0}
  },
  { // 9
    {0, 1, 1, 1, 0}, 
    {1, 0, 0, 0, 1}, 
    {1, 0, 0, 0, 1}, 
    {0, 1, 1, 1, 1}, 
    {0, 0, 0, 0, 1}, 
    {0, 0, 0, 1, 0}, 
    {0, 0, 1, 0, 0}
  }
};





const bool char_list[CHAR_N_LIST][CHAR_HEIGHT][CHAR_WIDTH] = {
  { // a
    {0, 0, 0, 0, 0}, 
    {0, 0, 0, 0, 0}, 
    {0, 1, 1, 1, 0}, 
    {0, 0, 0, 0, 1}, 
    {0, 1, 1, 1, 1}, 
    {1, 0, 0, 0, 1}, 
    {0, 1, 1, 1, 1}
  },
  { // d
    {0, 0, 0, 0, 0}, 
    {0, 0, 0, 0, 1}, 
    {0, 0, 0, 0, 1}, 
    {0, 1, 1, 1, 1}, 
    {1, 0, 0, 0, 1}, 
    {1, 0, 0, 1, 1}, 
    {0, 1, 1, 0, 1}
  },
  { // e
    {0, 0, 0, 0, 0}, 
    {0, 0, 0, 0, 0}, 
    {0, 1, 1, 1, 0}, 
    {1, 0, 0, 0, 1}, 
    {1, 1, 1, 1, 1}, 
    {1, 0, 0, 0, 0}, 
    {0, 1, 1, 1, 0}
  },
  { // h
    {1, 0, 0, 0, 0}, 
    {1, 0, 0, 0, 0}, 
    {1, 0, 1, 1, 0}, 
    {1, 1, 0, 0, 1}, 
    {1, 0, 0, 0, 1}, 
    {1, 0, 0, 0, 1}, 
    {1, 0, 0, 0, 1}
  },
  { // i
    {0, 0, 0, 0, 0}, 
    {0, 0, 1, 0, 0}, 
    {0, 0, 0, 0, 0}, 
    {0, 1, 1, 0, 0}, 
    {0, 0, 1, 0, 0}, 
    {0, 0, 1, 0, 0}, 
    {0, 1, 1, 1, 0}
  },
  { // m
    {0, 0, 0, 0, 0}, 
    {0, 0, 0, 0, 0}, 
    {1, 1, 0, 1, 0}, 
    {1, 0, 1, 0, 1}, 
    {1, 0, 1, 0, 1}, 
    {1, 0, 0, 0, 1}, 
    {1, 0, 0, 0, 1}
  },
  { // p
    {0, 0, 0, 0, 0}, 
    {0, 0, 0, 0, 0}, 
    {1, 1, 1, 1, 0}, 
    {1, 0, 0, 0, 1}, 
    {1, 1, 1, 1, 0}, 
    {1, 0, 0, 0, 0}, 
    {1, 0, 0, 0, 0}
  },
  { // r
    {0, 0, 0, 0, 0}, 
    {0, 0, 0, 0, 0}, 
    {1, 0, 1, 1, 1}, 
    {1, 1, 0, 0, 0}, 
    {1, 0, 0, 0, 0}, 
    {1, 0, 0, 0, 0}, 
    {1, 0, 0, 0, 0}
  },
  { // s
    {0, 0, 0, 0, 0}, 
    {0, 0, 0, 0, 0}, 
    {0, 1, 1, 1, 1}, 
    {1, 0, 0, 0, 0}, 
    {0, 1, 1, 1, 0}, 
    {0, 0, 0, 0, 1}, 
    {1, 1, 1, 1, 0}
  },
  { // t
    {0, 0, 0, 0, 0}, 
    {0, 1, 0, 0, 0}, 
    {1, 1, 1, 1, 1}, 
    {0, 1, 0, 0, 0}, 
    {0, 1, 0, 0, 0}, 
    {0, 1, 0, 0, 1}, 
    {0, 0, 1, 1, 0}
  },
  { // u
    {0, 0, 0, 0, 0}, 
    {0, 0, 0, 0, 0}, 
    {1, 0, 0, 0, 1}, 
    {1, 0, 0, 0, 1}, 
    {1, 0, 0, 0, 1}, 
    {1, 0, 0, 1, 1}, 
    {0, 1, 1, 0, 1}
  },
  { // y
    {0, 0, 0, 0, 0}, 
    {0, 0, 0, 0, 0}, 
    {1, 0, 0, 0, 1}, 
    {1, 0, 0, 0, 1}, 
    {0, 1, 0, 1, 0}, 
    {0, 0, 1, 0, 0}, 
    {1, 1, 0, 0, 0}
  },
  { // C
    {0, 1, 1, 1, 0}, 
    {1, 0, 0, 0, 1}, 
    {1, 0, 0, 0, 0}, 
    {1, 0, 0, 0, 0}, 
    {1, 0, 0, 0, 0}, 
    {1, 0, 0, 0, 1}, 
    {0, 1, 1, 1, 0}
  },
  { // H
    {1, 0, 0, 0, 1}, 
    {1, 0, 0, 0, 1}, 
    {1, 0, 0, 0, 1}, 
    {1, 1, 1, 1, 1}, 
    {1, 0, 0, 0, 1}, 
    {1, 0, 0, 0, 1}, 
    {1, 0, 0, 0, 1}
  },
  { // O
    {0, 1, 1, 1, 0}, 
    {1, 0, 0, 0, 1}, 
    {1, 0, 0, 0, 1}, 
    {1, 0, 0, 0, 1}, 
    {1, 0, 0, 0, 1}, 
    {1, 0, 0, 0, 1}, 
    {0, 1, 1, 1, 0}
  },
  { // P
    {1, 1, 1, 1, 0}, 
    {1, 0, 0, 0, 1}, 
    {1, 0, 0, 0, 1}, 
    {1, 1, 1, 1, 0}, 
    {1, 0, 0, 0, 0}, 
    {1, 0, 0, 0, 0}, 
    {1, 0, 0, 0, 0}
  },
  { // T
    {1, 1, 1, 1, 1}, 
    {0, 0, 1, 0, 0}, 
    {0, 0, 1, 0, 0}, 
    {0, 0, 1, 0, 0}, 
    {0, 0, 1, 0, 0}, 
    {0, 0, 1, 0, 0}, 
    {0, 0, 1, 0, 0}
  },
  { // 2
    {0, 1, 1, 1, 0}, 
    {1, 0, 0, 0, 1}, 
    {0, 0, 0, 0, 1}, 
    {0, 0, 0, 1, 0}, 
    {0, 0, 1, 0, 0}, 
    {0, 1, 0, 0, 0}, 
    {1, 1, 1, 1, 1}
  },
  { // /
    {0, 0, 0, 0, 0}, 
    {0, 0, 0, 0, 1}, 
    {0, 0, 0, 1, 0}, 
    {0, 0, 1, 0, 0}, 
    {0, 1, 0, 0, 0}, 
    {1, 0, 0, 0, 0}, 
    {0, 0, 0, 0, 0}
  },
  { // :
    {0, 0, 0, 0, 0}, 
    {0, 1, 1, 0, 0}, 
    {0, 1, 1, 0, 0}, 
    {0, 0, 0, 0, 0}, 
    {0, 1, 1, 0, 0}, 
    {0, 1, 1, 0, 0}, 
    {0, 0, 0, 0, 0}
  },
  { // *C (special character)
    {1, 0, 0, 0, 0}, 
    {0, 0, 1, 1, 0}, 
    {0, 1, 0, 0, 1}, 
    {0, 1, 0, 0, 0}, 
    {0, 1, 0, 0, 0}, 
    {0, 1, 0, 0, 1}, 
    {0, 0, 1, 1, 0}
  },
  { // %
    {1, 1, 0, 0, 0}, 
    {1, 1, 0, 0, 1}, 
    {0, 0, 0, 1, 0}, 
    {0, 0, 1, 0, 0}, 
    {0, 1, 0, 0, 0}, 
    {1, 0, 0, 1, 1}, 
    {0, 0, 0, 1, 1}
  },
};


/*
{ // 
  {0, 0, 0, 0, 0}, 
  {0, 0, 0, 0, 0}, 
  {0, 0, 0, 0, 0}, 
  {0, 0, 0, 0, 0}, 
  {0, 0, 0, 0, 0}, 
  {0, 0, 0, 0, 0}, 
  {0, 0, 0, 0, 0}
},
  { // 
    {0, 0, 0, 0, 0}, 
    {0, 0, 0, 0, 0}, 
    {0, 0, 0, 0, 0}, 
    {0, 0, 0, 0, 0}, 
    {0, 0, 0, 0, 0}, 
    {0, 0, 0, 0, 0}, 
    {0, 0, 0, 0, 0}
  },
*/

#endif