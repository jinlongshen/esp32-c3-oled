#ifndef COMPONENT_LVGL_DRIVER_LVGL_DISPLAY_H
#define COMPONENT_LVGL_DRIVER_LVGL_DISPLAY_H

#include "ssd1306.h"

namespace muc::lvgl_driver
{

// LVGL visible area = OLED glass area
constexpr std::size_t LVGL_WIDTH = 72;
constexpr std::size_t LVGL_HEIGHT = 40;
constexpr std::size_t LV_BUF_PIXELS = (LVGL_WIDTH * LVGL_HEIGHT) / 8;
constexpr std::size_t LV_BUF_SIZE = LV_BUF_PIXELS + 8;

void lvgl_display_init(muc::ssd1306::Oled& oled);
} // namespace muc::lvgl_driver

#endif // COMPONENT_LVGL_DRIVER_LVGL_DISPLAY_H
