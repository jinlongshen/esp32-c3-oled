#include "lvgl_driver.h"
#include "lvgl_display.h"
#include "lvgl.h"
#include "display_geometry.h"

namespace muc::lvgl_driver
{

void lvgl_driver_init(muc::ssd1306::Oled& oled) noexcept
{
    lv_init();

    // Create LVGL display using compile‑time geometry
    lv_display_t* disp = lv_display_create(muc::ssd1306::kDefaultGeometry.width,
                                           muc::ssd1306::kDefaultGeometry.height);

    // Dummy draw buffer (unused)
    lv_draw_buf_t dummy{};

    // Initialize LVGL display driver
    init_display(*disp, dummy, oled, nullptr, 0);
}

} // namespace muc::lvgl_driver
