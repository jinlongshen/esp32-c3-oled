#ifndef COMPONENT_LVGL_DRIVER_LVGL_DRIVER_H
#define COMPONENT_LVGL_DRIVER_LVGL_DRIVER_H

#include "ssd1306.h"

namespace muc::lvgl_driver
{

/**
 * Initialize the LVGL display driver for the given SSD1306 OLED.
 *
 * This:
 *  - Creates an LVGL display with geometry from oled.geometry()
 *  - Sets LV_COLOR_FORMAT_I1
 *  - Installs the flush callback
 *  - Sets up the LVGL draw buffer
 *  - Performs an optional hardware test pattern
 */
void lvgl_driver_init(muc::ssd1306::Oled& oled);

} // namespace muc::lvgl_driver

#endif // COMPONENT_LVGL_DRIVER_LVGL_DRIVER_H
