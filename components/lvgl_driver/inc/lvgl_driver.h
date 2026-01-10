#ifndef COMPONENT_LVGL_DRIVER_H
#define COMPONENT_LVGL_DRIVER_H

#include <cstdint>
#include "lvgl.h"
#include "ssd1306.h"

namespace muc::lvgl_driver
{

constexpr bool ENABLE_HANDLE_TEST = false;

void lvgl_driver_init(muc::ssd1306::Oled& oled);

} // namespace muc::lvgl_driver

#endif //
