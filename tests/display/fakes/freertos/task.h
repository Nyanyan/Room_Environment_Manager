#pragma once
#include "FreeRTOS.h"

inline void (*fake_task_entry)(void *) = nullptr;
inline int fake_task_creations = 0;
inline int fake_task_cycles = 0;
inline std::function<void()> fake_before_tick;
struct TaskStopped {};

inline TaskHandle_t xTaskCreateStaticPinnedToCore(void (*entry)(void *), const char *, uint32_t stack_size,
                                                void *, int priority, StackType_t *stack,
                                                StaticTask_t *task, int core) {
  assert(stack && stack_size >= 3072 && priority > 0 && core == ARDUINO_RUNNING_CORE);
  ++fake_task_creations;
  fake_task_entry = entry;
  return task;
}
inline TickType_t xTaskGetTickCount() { return fake_now; }
inline void vTaskDelayUntil(TickType_t *wake, TickType_t interval) {
  assert(interval == 1000 && fake_critical_depth == 0);
  if (fake_before_tick) fake_before_tick();
  *wake += interval;
  fake_now = *wake;
  if (--fake_task_cycles <= 0) throw TaskStopped{};
}
