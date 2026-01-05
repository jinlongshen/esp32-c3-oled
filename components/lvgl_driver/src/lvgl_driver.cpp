#include "lvgl_driver.h"
#include <array>
#include <cstdint>
#include <cstdio>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ssd1306.h"

namespace
{
muc::ssd1306::Oled* g_oled{nullptr};

// Dimensions of your physical glass
constexpr std::uint32_t VISIBLE_W = 72;
constexpr std::uint32_t VISIBLE_H = 40;

// Dimensions of the SSD1306 RAM
constexpr std::uint32_t CTRL_W = 128;
constexpr std::uint32_t CTRL_H = 64;

// Your verified offsets
constexpr std::uint32_t OFF_X = 27;
constexpr std::uint32_t OFF_Y = 24;

// LVGL Buffer (72x40 area + 8 byte palette)
constexpr std::uint32_t LV_BUF_SIZE = (VISIBLE_W * VISIBLE_H / 8) + 8;
static std::array<std::uint8_t, LV_BUF_SIZE> g_draw_buf;

// Controller RAM Buffer (128x64 = 1024 bytes)
static std::array<std::uint8_t, 1024> g_vtiled_buf;

void convert_to_ssd1306_format(const std::uint8_t* src_pixels)
{
    g_vtiled_buf.fill(0); // Clear RAM buffer (Black)

    for (std::uint32_t y = 0; y < VISIBLE_H; ++y)
    {
        for (std::uint32_t x = 0; x < VISIBLE_W; ++x)
        {
            // Extract bit from LVGL 72-wide buffer
            std::uint32_t src_byte_idx = (y * VISIBLE_W + x) / 8;
            std::uint8_t src_bit_mask = 1 << (7 - (x % 8));

            if (src_pixels[src_byte_idx] & src_bit_mask)
            {
                // Shift to physical glass location
                std::uint32_t phys_x = x + OFF_X;
                std::uint32_t phys_y = y + OFF_Y;

                std::uint32_t page = phys_y / 8;
                std::uint32_t bit_in_page = phys_y % 8;

                // Set bit in the 128-column wide controller map
                g_vtiled_buf[page * CTRL_W + phys_x] |= (1 << bit_in_page);
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
    printf("[LVGL] Starting Driver Init (72x40 Window)...\n");

    // --- HARDWARE TEST: Turn glass WHITE for 500ms ---
    g_vtiled_buf.fill(0xFF);
    g_oled->blitLVGLBuffer(g_vtiled_buf);
    g_oled->update();
    vTaskDelay(pdMS_TO_TICKS(500));

    // Clear back to black
    g_vtiled_buf.fill(0x00);
    g_oled->blitLVGLBuffer(g_vtiled_buf);
    g_oled->update();

    lv_init();

    // Create display for the 72x40 area
    auto* disp = lv_display_create(VISIBLE_W, VISIBLE_H);
    lv_display_set_buffers(
        disp, g_draw_buf.data(), nullptr, g_draw_buf.size(), LV_DISPLAY_RENDER_MODE_FULL);
    lv_display_set_color_format(disp, LV_COLOR_FORMAT_I1);

    lv_display_set_flush_cb(disp,
                            [](lv_display_t* d, const lv_area_t* a, std::uint8_t* px)
                            {
                                // Skip 8-byte palette and convert
                                convert_to_ssd1306_format(px + 8);
                                if (g_oled)
                                {
                                    g_oled->blitLVGLBuffer(g_vtiled_buf);
                                    g_oled->update();
                                }
                                lv_display_flush_ready(d);
                            });

    // --- TEST UI ---
    lv_obj_t* scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, lv_color_white(), 0);

    // Add a 1px Black Border around the LVGL edge
    static lv_style_t style_border;
    lv_style_init(&style_border);
    lv_style_set_border_width(&style_border, 1);
    lv_style_set_border_color(&style_border, lv_color_black());
    lv_obj_add_style(scr, &style_border, 0);

    lv_obj_t* label = lv_label_create(scr);
    lv_label_set_text(label, "ALIGNED");
    lv_obj_set_style_text_color(label, lv_color_black(), 0);
    lv_obj_center(label);

    printf("[LVGL] Init Complete.\n");
}

void lvgl_tick_task(void* pvArgument) noexcept
{
    (void)pvArgument;
    while (true)
    {
        lv_tick_inc(5);
        lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

} // namespace muc::lvgl_driver