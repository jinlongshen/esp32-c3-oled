#include "lvgl_driver.h"

#include "lvgl_display.h"
#include "lvgl_tasks.h"
#include "lvgl.h"

namespace muc::lvgl_driver
{

void init(muc::ssd1306::Oled& oled) noexcept
{
    lv_init();
    lvgl_display_init(oled);
}

} // namespace muc::lvgl_driver
