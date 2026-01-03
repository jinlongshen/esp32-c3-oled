#ifndef COMPONENTS_LVGL_DRIVER_H
#define COMPONENTS_LVGL_DRIVER_H

#include "lvgl/lvgl.h"

namespace muc::lvgl_driver
{
void init() noexcept;
void lvgl_tick_task(void* pvArgument) noexcept;
} // namespace muc::lvgl_driver

#endif // COMPONENTS_LVGL_DRIVER_H
