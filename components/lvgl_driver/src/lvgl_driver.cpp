#include "lvgl_driver.h"

#include <array>
#include <cstdint>
#include <cstdio>
#include <span>
#include <algorithm>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "display_geometry.h"

// #include "lv_font_custom_12.h"

namespace muc::lvgl_driver
{

using muc::ssd1306::kDefaultGeometry;
using muc::ssd1306::kVisibleFramebufferBytes;
using muc::ssd1306::kVisibleLvglBufferBytes;

// -----------------------------------------------------------------------------
// Static LVGL buffers and draw descriptors - single buffer mode
// -----------------------------------------------------------------------------
static std::array<std::uint8_t, kVisibleLvglBufferBytes> s_buf1{};
static lv_draw_buf_t s_draw_buf1;

static muc::ssd1306::Oled* s_oled = nullptr;

// -----------------------------------------------------------------------------
// LVGL flush callback (I1 format)
// -----------------------------------------------------------------------------
static void flush_cb(lv_display_t* disp, const lv_area_t*, std::uint8_t* color_p)
{
    auto* oled = static_cast<muc::ssd1306::Oled*>(lv_display_get_user_data(disp));
    if (!oled)
    {
        lv_display_flush_ready(disp);
        return;
    }

    const std::uint8_t* src = color_p + 8; // skip palette
    oled->blitLVGLBuffer(std::span<const std::uint8_t>(src, kVisibleFramebufferBytes));
    oled->update();
    lv_display_flush_ready(disp);
}

// -----------------------------------------------------------------------------
// LVGL display initialization
// -----------------------------------------------------------------------------
static void init_display(lv_display_t& disp, muc::ssd1306::Oled& oled)
{
    s_oled = &oled;

    const auto& g = oled.geometry();
    if (g.width != kDefaultGeometry.width || g.height != kDefaultGeometry.height)
    {
        std::printf("[LVGL][WARN] Geometry mismatch: OLED=%dx%d expected=%dx%d\n",
                    g.width,
                    g.height,
                    kDefaultGeometry.width,
                    kDefaultGeometry.height);
    }

    if constexpr (ENABLE_HANDLE_TEST)
    {
        std::array<std::uint8_t, kVisibleFramebufferBytes> test{};
        std::fill(test.begin(), test.end(), 0xFF);
        oled.blitLVGLBuffer(test);
        oled.update();
        vTaskDelay(pdMS_TO_TICKS(200));

        std::fill(test.begin(), test.end(), 0x00);
        oled.blitLVGLBuffer(test);
        oled.update();
        vTaskDelay(pdMS_TO_TICKS(200));
    }

    // Initialize LVGL draw buffers
    lv_draw_buf_init(&s_draw_buf1,
                     kDefaultGeometry.width,
                     kDefaultGeometry.height,
                     LV_COLOR_FORMAT_I1,
                     0,
                     s_buf1.data(),
                     static_cast<std::uint32_t>(s_buf1.size()));

    lv_display_set_draw_buffers(&disp, &s_draw_buf1, nullptr);
    lv_display_set_color_format(&disp, LV_COLOR_FORMAT_I1);
    lv_display_set_flush_cb(&disp, flush_cb);
    lv_display_set_user_data(&disp, s_oled);

#if 0
    // Simple LVGL test
    lv_obj_t* scr = lv_screen_active();
    lv_obj_t* label = lv_label_create(scr);
    // lv_obj_set_style_text_font(label, &lv_font_custom_12, 0);
    // lv_label_set_text(label, "元旦结束了");
    lv_label_set_text(label, "Hello");
    lv_obj_center(label);
#endif

    std::printf("[LVGL] Init complete (%dx%d, LVGL buf=%zu, FB=%zu)\n",
                kDefaultGeometry.width,
                kDefaultGeometry.height,
                kVisibleLvglBufferBytes,
                kVisibleFramebufferBytes);
}

// -----------------------------------------------------------------------------
// Public driver initialization
// -----------------------------------------------------------------------------
void lvgl_driver_init(muc::ssd1306::Oled& oled)
{
    lv_init();
    lv_display_t* disp = lv_display_create(kDefaultGeometry.width, kDefaultGeometry.height);
    // Force full-frame rendering, no partial flushes
    lv_display_set_render_mode(disp, LV_DISPLAY_RENDER_MODE_FULL);
    init_display(*disp, oled);
}

} // namespace muc::lvgl_driver
