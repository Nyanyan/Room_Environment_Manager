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
#define CHAR_TITLE_MARGIN_X 3
#define CHAR_TITLE_MARGIN_Y 2
#define CHAR_TEMPERATURE_N 13 // Temperature *C
#define CHAR_HUMIDITY_N 10 // Humidity %
#define CHAR_PRESSURE_N 12 // Pressure hPa
#define CHAR_CO2_CONCENTRATION_N 7 // CO2 ppm
#define CHAR_THI_N 3 // THI

// graph x unit
#define CHAR_TIME_N 4 // time
#define CHAR_UNIT_X_EY 3
#define CHAR_UNIT_X_EX (GRAPH_SX - 7)

// alphabet
#define CHAR_N_LIST 33
#define CHAR_LIST_2 2
#define CHAR_LIST_a 10
#define CHAR_LIST_d 11
#define CHAR_LIST_e 12
#define CHAR_LIST_h 13
#define CHAR_LIST_i 14
#define CHAR_LIST_m 15
#define CHAR_LIST_p 16
#define CHAR_LIST_r 17
#define CHAR_LIST_s 18
#define CHAR_LIST_t 19
#define CHAR_LIST_u 20
#define CHAR_LIST_y 21
#define CHAR_LIST_C 22
#define CHAR_LIST_H 23
#define CHAR_LIST_I 24
#define CHAR_LIST_O 25
#define CHAR_LIST_P 26
#define CHAR_LIST_T 27
#define CHAR_LIST_SPACE 28
#define CHAR_LIST_SLASH 29
#define CHAR_LIST_COLON 30
#define CHAR_LIST_DEGREE 31
#define CHAR_LIST_PERCENT 32

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
  CHAR_LIST_e,
  CHAR_LIST_SPACE,
  CHAR_LIST_DEGREE
};

const int char_idx_humidity[CHAR_HUMIDITY_N] = {
  CHAR_LIST_H, 
  CHAR_LIST_u, 
  CHAR_LIST_m, 
  CHAR_LIST_i, 
  CHAR_LIST_d, 
  CHAR_LIST_i, 
  CHAR_LIST_t, 
  CHAR_LIST_y,
  CHAR_LIST_SPACE,
  CHAR_LIST_PERCENT
};

const int char_idx_pressure[CHAR_PRESSURE_N] = {
  CHAR_LIST_P, 
  CHAR_LIST_r, 
  CHAR_LIST_e, 
  CHAR_LIST_s, 
  CHAR_LIST_s, 
  CHAR_LIST_u, 
  CHAR_LIST_r, 
  CHAR_LIST_e,
  CHAR_LIST_SPACE,
  CHAR_LIST_h,
  CHAR_LIST_P,
  CHAR_LIST_a
};

const int char_idx_co2_concentration[CHAR_CO2_CONCENTRATION_N] = {
  CHAR_LIST_C, 
  CHAR_LIST_O, 
  CHAR_LIST_2,
  CHAR_LIST_SPACE,
  CHAR_LIST_p,
  CHAR_LIST_p,
  CHAR_LIST_m
};

const int char_idx_thi[CHAR_THI_N] = {
  CHAR_LIST_T, 
  CHAR_LIST_H, 
    CHAR_LIST_I
};

const int char_idx_time[CHAR_TIME_N] = {
  CHAR_LIST_t, 
  CHAR_LIST_i, 
  CHAR_LIST_m, 
  CHAR_LIST_e
};





const bool char_list[CHAR_N_LIST][CHAR_HEIGHT][CHAR_WIDTH] = {
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
  },
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
  { // I
    {0, 1, 1, 1, 0}, 
    {0, 0, 1, 0, 0}, 
    {0, 0, 1, 0, 0}, 
    {0, 0, 1, 0, 0}, 
    {0, 0, 1, 0, 0}, 
    {0, 0, 1, 0, 0}, 
    {0, 1, 1, 1, 0}
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
  { // SPACE
    {0, 0, 0, 0, 0}, 
    {0, 0, 0, 0, 0}, 
    {0, 0, 0, 0, 0}, 
    {0, 0, 0, 0, 0}, 
    {0, 0, 0, 0, 0}, 
    {0, 0, 0, 0, 0}, 
    {0, 0, 0, 0, 0}
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