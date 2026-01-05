#include "lvgl_driver.h"
#include <array>
#include <cstdint>
#include <span>
#include <cstdio>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace
{
muc::ssd1306::Oled* g_oled{nullptr};
constexpr uint32_t W = 128;
constexpr uint32_t H = 64;

// LVGL 9 I1 format: (Size/8) + 8 byte palette
constexpr uint32_t LV_BUF_SIZE = (W * H / 8) + 8;
static std::array<uint8_t, LV_BUF_SIZE> g_draw_buf;
static std::array<uint8_t, (W * H / 8)> g_vtiled_buf;

/**
 * Converts LVGL I1 (Horizontal bit-packed) to SSD1306 (Vertical page-packed)
 */
void convert_to_ssd1306_format(const uint8_t* src_pixels)
{
    g_vtiled_buf.fill(0);
    // LVGL I1: Each byte is 8 horizontal pixels. MSB is leftmost.
    // SSD1306: Each byte is 8 vertical pixels (1 page). LSB is topmost.

    for (uint32_t y = 0; y < H; ++y)
    {
        for (uint32_t x = 0; x < W; ++x)
        {
            uint32_t src_byte_idx = (y * W + x) / 8;
            uint8_t src_bit_idx = 7 - (x % 8);

            if (src_pixels[src_byte_idx] & (1 << src_bit_idx))
            {
                // Determine which page (row of 8 pixels high) and which bit in that page
                uint32_t page = y / 8;
                uint32_t bit_in_page = y % 8;
                g_vtiled_buf[page * W + x] |= (1 << bit_in_page);
            }
        }
    }
}
} // namespace

namespace muc::lvgl_driver
{

void init(muc::ssd1306::Oled& oled) noexcept
{
    g_oled = &oled;
    printf("[LVGL] Starting Driver Init...\n");

    // --- STEP 1: HARDWARE SANITY CHECK ---
    // Bypass LVGL and turn the screen WHITE immediately to verify I2C/SPI
    printf("[LVGL] Performing Hardware Sanity Test (White Screen)...\n");
    g_vtiled_buf.fill(0xFF);
    g_oled->blitLVGLBuffer(g_vtiled_buf);
    g_oled->update();

    // --- STEP 2: LVGL CORE INIT ---
    lv_init();

    auto* disp = lv_display_create(W, H);
    lv_display_set_buffers(
        disp, g_draw_buf.data(), nullptr, g_draw_buf.size(), LV_DISPLAY_RENDER_MODE_FULL);
    lv_display_set_color_format(disp, LV_COLOR_FORMAT_I1);

    // Set the Flush Callback
    lv_display_set_flush_cb(disp,
                            [](lv_display_t* d, const lv_area_t* a, uint8_t* px)
                            {
                                // px points to g_draw_buf + 8 (the actual pixel data)
                                // We pass the pointer starting after the 8-byte palette
                                convert_to_ssd1306_format(px + 8);

                                if (g_oled)
                                {
                                    g_oled->blitLVGLBuffer(g_vtiled_buf);
                                    g_oled->update();
                                }
                                lv_display_flush_ready(d);
                            });

    // --- STEP 3: CREATE TEST UI ---
    lv_obj_t* scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, lv_color_white(), LV_PART_MAIN);

    lv_obj_t* label = lv_label_create(scr);
    lv_label_set_text(label, "LVGL 9 UP");
    lv_obj_set_style_text_color(label, lv_color_black(), LV_PART_MAIN);
    lv_obj_center(label);

    printf("[LVGL] Driver Init Complete.\n");
}

void lvgl_tick_task(void* pvArgument) noexcept
{
    (void)pvArgument;
    printf("[LVGL] Tick Task Started.\n");
    while (true)
    {
        lv_tick_inc(5);
        lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

} // namespace muc::lvgl_driver