#ifndef COMPONENTS_HOOKS_H
#define COMPONENTS_HOOKS_H

#include <cstdint>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// All these functions are C symbols that FreeRTOS / ESP-IDF expects,
// so we must keep them in an extern "C" block when included from C++.
#ifdef __cplusplus
extern "C"
{
#endif

// Called by FreeRTOS when a stack overflow is detected in any task.
void vApplicationStackOverflowHook(TaskHandle_t xTask, char* pcTaskName);

// Called by FreeRTOS when the idle task runs (CPU0 idle).
void vApplicationIdleHook(void);

// Our own debug/monitoring task that periodically logs stack usage.
void stack_monitor_task(void* arg);

#ifdef __cplusplus
}
#endif

#endif // COMPONENTS_HOOKS_H
