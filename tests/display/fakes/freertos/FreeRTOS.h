#pragma once
#include <Arduino.h>
using TickType_t = uint32_t;
using StackType_t = uint8_t;
struct StaticTask_t {};
using TaskHandle_t = StaticTask_t *;
constexpr int ARDUINO_RUNNING_CORE = 1;
#define pdMS_TO_TICKS(ms) (ms)
#define configASSERT(condition) assert(condition)
