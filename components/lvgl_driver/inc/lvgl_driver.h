#ifndef COMPONENTS_LVGL_DRIVER_H
#define COMPONENTS_LVGL_DRIVER_H

#include "ssd1306.h"

namespace muc::lvgl_driver
{
void init(muc::ssd1306::Oled& oled) noexcept;
void lvgl_tick_task(void* arg) noexcept;
void lvgl_handler_task(void* pvArgument) noexcept;
} // namespace muc::lvgl_driver

#endif // COMPONENTS_LVGL_DRIVER_H
