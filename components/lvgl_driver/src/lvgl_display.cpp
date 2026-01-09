#include "lvgl_display.h"
#include "ssd1306.h"
#include "lvgl.h"
#include <array>
#include <cstdint>
#include <cstdio>
#include <span>

namespace muc::lvgl_driver
{

// LVGL buffer: 1‑bit framebuffer + 8‑byte palette
std::array<std::uint8_t, LV_BUF_SIZE> g_draw_buf{};
muc::ssd1306::Oled* g_oled = nullptr;

// -----------------------------------------------------------------------------
// LVGL flush callback
// -----------------------------------------------------------------------------
static void lvgl_flush_cb(lv_display_t* disp, const lv_area_t* area, std::uint8_t* color_p)
{
    auto* oled = static_cast<muc::ssd1306::Oled*>(lv_display_get_user_data(disp));
    if (!oled)
    {
        lv_display_flush_ready(disp);
        return;
    }

    // LVGL gives us LVGL_W × LVGL_H I1 buffer + palette.
    oled->blitLVGLBuffer(std::span<const std::uint8_t>(color_p, LV_BUF_PIXELS));
    oled->update();

    lv_display_flush_ready(disp);
}

void lvgl_display_init(muc::ssd1306::Oled& oled)
{
    g_oled = &oled;

    // Optional hardware test
    {
        std::array<std::uint8_t, LV_BUF_PIXELS> test{};
        test.fill(0xFF);
        g_oled->blitLVGLBuffer(test);
        g_oled->update();
        vTaskDelay(pdMS_TO_TICKS(500));

        test.fill(0x00);
        g_oled->blitLVGLBuffer(test);
        g_oled->update();
    }

    // Create LVGL display for the visible area
    lv_display_t* disp = lv_display_create(LVGL_WIDTH, LVGL_HEIGHT);
    lv_display_set_color_format(disp, LV_COLOR_FORMAT_I1);
    lv_display_set_flush_cb(disp, lvgl_flush_cb);

    lv_display_set_buffers(
        disp, g_draw_buf.data(), nullptr, g_draw_buf.size(), LV_DISPLAY_RENDER_MODE_FULL);

    lv_display_set_user_data(disp, g_oled);

    // Simple LVGL test UI
    lv_obj_t* scr = lv_screen_active();
    lv_obj_t* label = lv_label_create(scr);
    lv_label_set_text(label, "Hello");
    lv_obj_center(label);
    lv_obj_set_layout(scr, LV_LAYOUT_NONE);
    lv_obj_set_style_pad_all(scr, 0, 0);

    std::printf("[LVGL] Init Complete.\n");
}

} // namespace muc::lvgl_driver
