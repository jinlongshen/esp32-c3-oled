#ifndef COMPONENT_LVGL_DRIVER_LVGL_TASKS_H
#define COMPONENT_LVGL_DRIVER_LVGL_TASKS_H

#include <cstdint>

namespace muc::lvgl_driver
{

// Configuration for LVGL timing (no stack/priority here)
struct LvglTaskConfig
{
    std::uint32_t tick_period_ms;
    std::uint32_t handler_period_ms;
};

// These are *task entry functions* only.
// The system is responsible for creating the FreeRTOS tasks.
void lvgl_tick_task(void* arg);
void lvgl_handler_task(void* arg);

} // namespace muc::lvgl_driver

#endif // COMPONENT_LVGL_DRIVER_LVGL_TASKS_H
