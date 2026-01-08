#include "lv_conf.h"
#include "lvgl.h"

#include "lvgl_driver.h"

#include <array>
#include <cstdint>
#include <cstdio>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "ssd1306.h"

namespace
{

muc::ssd1306::Oled* g_oled{nullptr};

// Logical LVGL display size = physical glass area
constexpr std::uint32_t VISIBLE_W = 72;
constexpr std::uint32_t VISIBLE_H = 40;

// LVGL buffer: 1-bit (I1) framebuffer + 8-byte palette
constexpr std::uint32_t LV_BUF_PIXELS = (VISIBLE_W * VISIBLE_H) / 8;
constexpr std::uint32_t LV_BUF_SIZE = LV_BUF_PIXELS + 8;

static std::array<std::uint8_t, LV_BUF_SIZE> g_draw_buf{};

// -----------------------------------------------------------------------------
// LVGL flush callback (safe minimal diagnostics)
// -----------------------------------------------------------------------------
static void my_flush_cb(lv_display_t* disp, const lv_area_t* area, uint8_t* color_p)
{
    ESP_LOGI("LVGL", "Flush called");
    auto* oled = static_cast<muc::ssd1306::Oled*>(lv_display_get_user_data(disp));
    if (!oled)
    {
        lv_display_flush_ready(disp);
        return;
    }

    // constexpr std::size_t buf_size = (muc::ssd1306::OLED_WIDTH * muc::ssd1306::OLED_HEIGHT) / 8;
    constexpr std::size_t buf_size = LV_BUF_PIXELS;

    oled->blitLVGLBuffer(std::span<const uint8_t>(color_p, buf_size));
    oled->update();

    lv_display_flush_ready(disp);
}

} // namespace

namespace muc::lvgl_driver
{

void init(muc::ssd1306::Oled& oled) noexcept
{
    g_oled = &oled;
    std::printf("[LVGL] Starting Driver Init (72x40 Window)...\n");

    // Optional: simple hardware test via the SSD1306 driver
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

    lv_init();

    // Create LVGL display for the 72x40 logical area
    lv_display_t* disp = lv_display_create(VISIBLE_W, VISIBLE_H);
    lv_display_set_color_format(disp, LV_COLOR_FORMAT_I1);
    lv_display_set_flush_cb(disp, my_flush_cb);

    lv_display_set_buffers(
        disp, g_draw_buf.data(), nullptr, g_draw_buf.size(), LV_DISPLAY_RENDER_MODE_FULL);

    // *** CRITICAL: wire OLED instance into LVGL display ***
    lv_display_set_user_data(disp, g_oled);
    // --- Simple test UI on the LVGL side ---
    lv_obj_t* scr = lv_screen_active();
    lv_obj_t* label = lv_label_create(scr);
    lv_label_set_text(label, "Hello");
    lv_obj_center(label);
    lv_obj_set_layout(scr, LV_LAYOUT_NONE);
    lv_obj_set_style_pad_all(scr, 0, 0);
    std::printf("[LVGL] Init Complete.\n");
}

// Fast tick, very cheap
void lvgl_tick_task(void* arg)
{
    const uint32_t period_ms = 10;

    while (true)
    {
        lv_tick_inc(period_ms);
        vTaskDelay(pdMS_TO_TICKS(period_ms));
    }
}

// UI/task context, but still should not block on I2C
void lvgl_handler_task(void* arg) noexcept
{

    while (true)
    {
        lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

} // namespace muc::lvgl_driver
