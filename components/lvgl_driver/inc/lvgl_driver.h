#ifndef COMPONENT_LVGL_DRIVER_H
#define COMPONENT_LVGL_DRIVER_H

#include <cstdint>
#include "lvgl.h"
#include "ssd1306.h"

namespace muc::lvgl_driver
{

struct LvglTaskConfig
{
    std::uint32_t tick_period_ms;
    std::uint32_t handler_period_ms;
};

constexpr bool ENABLE_HANDLE_TEST = false;

void lvgl_driver_init(muc::ssd1306::Oled& oled);
void lvgl_tick_task(void* arg);
void lvgl_handler_task(void* arg);

} // namespace muc::lvgl_driver

#endif //
