#include "lvgl_display.h"

#include <array>
#include <cstdint>
#include <cstdio>
#include <span>
#include <algorithm>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "display_geometry.h"
#include "ssd1306.h"
#include "lvgl.h"

namespace muc::lvgl_driver
{

using muc::ssd1306::kDefaultGeometry;
using muc::ssd1306::kVisibleFramebufferBytes;
using muc::ssd1306::kVisibleLvglBufferBytes;

// Two LVGL I1 buffers (8‑byte palette + visible framebuffer)
static std::array<std::uint8_t, kVisibleLvglBufferBytes> s_buf1{};
static std::array<std::uint8_t, kVisibleLvglBufferBytes> s_buf2{};

// LVGL draw buffer descriptors
static lv_draw_buf_t s_draw_buf1;
static lv_draw_buf_t s_draw_buf2;

// Pointer to the OLED driver
static muc::ssd1306::Oled* s_oled = nullptr;

// -----------------------------------------------------------------------------
// LVGL flush callback (I1 format)
// -----------------------------------------------------------------------------
static void lvgl_flush_cb(lv_display_t* disp,
                          [[maybe_unused]] const lv_area_t* area,
                          std::uint8_t* color_p)
{
    auto* oled = static_cast<muc::ssd1306::Oled*>(lv_display_get_user_data(disp));
    if (!oled)
    {
        lv_display_flush_ready(disp);
        return;
    }

    // LVGL I1: first 8 bytes = palette
    const std::uint8_t* src = color_p + 8;

    oled->blitLVGLBuffer(std::span<const std::uint8_t>(src, kVisibleFramebufferBytes));
    oled->update();

    lv_display_flush_ready(disp);
}

// -----------------------------------------------------------------------------
// LVGL display initialization
// -----------------------------------------------------------------------------
void init_display(lv_display_t& disp,
                  lv_draw_buf_t& draw_buf_unused,
                  muc::ssd1306::Oled& oled,
                  std::uint8_t* buf_memory_unused,
                  std::size_t buf_size_unused)
{
    (void)draw_buf_unused;
    (void)buf_memory_unused;
    (void)buf_size_unused;

    s_oled = &oled;

    // Optional sanity check
    const auto& g = oled.geometry();
    if (g.width != kDefaultGeometry.width || g.height != kDefaultGeometry.height)
    {
        std::printf("[LVGL][WARN] Oled geometry (%dx%d) != kDefaultGeometry (%dx%d)\n",
                    g.width,
                    g.height,
                    kDefaultGeometry.width,
                    kDefaultGeometry.height);
    }

    // Optional hardware test
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

    lv_draw_buf_init(&s_draw_buf2,
                     kDefaultGeometry.width,
                     kDefaultGeometry.height,
                     LV_COLOR_FORMAT_I1,
                     0,
                     s_buf2.data(),
                     static_cast<std::uint32_t>(s_buf2.size()));

    // Attach buffers
    lv_display_set_draw_buffers(&disp, &s_draw_buf1, &s_draw_buf2);

    // Configure display
    lv_display_set_color_format(&disp, LV_COLOR_FORMAT_I1);
    lv_display_set_flush_cb(&disp, lvgl_flush_cb);
    lv_display_set_user_data(&disp, s_oled);

    // Simple LVGL test UI
    lv_obj_t* scr = lv_screen_active();
    lv_obj_t* label = lv_label_create(scr);
    lv_label_set_text(label, "Hello");
    lv_obj_center(label);

    std::printf("[LVGL] Init complete (%dx%d, LVGL buf=%zu, FB=%zu).\n",
                kDefaultGeometry.width,
                kDefaultGeometry.height,
                kVisibleLvglBufferBytes,
                kVisibleFramebufferBytes);
}

} // namespace muc::lvgl_driver
