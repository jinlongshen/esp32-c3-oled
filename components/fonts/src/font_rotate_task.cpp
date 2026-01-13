#include <esp_log.h>

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "FontRenderer.h"
#include "ssd1306.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace
{
const char* TAG = "FontExample";

extern "C"
{
extern const std::uint8_t _binary_oled_subset_ascii_umlaut_ttf_start[] asm(
    "_binary_oled_subset_ascii_umlaut_ttf_start");
extern const std::uint8_t _binary_oled_subset_ascii_umlaut_ttf_end[] asm(
    "_binary_oled_subset_ascii_umlaut_ttf_end");
}
} // namespace

namespace muc::fonts
{
void font_rotate_task(void* pvParameters)
{
    configASSERT(pvParameters && "font_rotate_task: pvParameters is nullptr");
    auto& oled = *static_cast<muc::ssd1306::Oled*>(pvParameters);
    muc::fonts::FontRenderer renderer;

    const std::size_t font_size =
        reinterpret_cast<std::uintptr_t>(_binary_oled_subset_ascii_umlaut_ttf_end) -
        reinterpret_cast<std::uintptr_t>(_binary_oled_subset_ascii_umlaut_ttf_start);

    if (!renderer.init(_binary_oled_subset_ascii_umlaut_ttf_start, font_size, 16))
    {
        ESP_LOGE(TAG, "Failed to initialize FontRenderer");
        vTaskDelete(nullptr);
    }

    double angle_deg = 0.0;
    const char* display_text = "Grüße";

    while (true)
    {
        oled.clear();

        // ---------------------------------------------------------------------
        // PASS 1: Compute bounding box
        // ---------------------------------------------------------------------
        double minX = 1e9, maxX = -1e9;
        double minY = 1e9, maxY = -1e9;

        const char* p_measure = display_text;
        double pen_x_measure = 0.0;

        while (*p_measure)
        {
            std::uint32_t cp = decode_utf8(p_measure);
            FT_Load_Char(renderer.face, cp, FT_LOAD_RENDER);
            FT_GlyphSlot g = renderer.face->glyph;

            if (g->bitmap.width > 0 && g->bitmap.rows > 0)
            {
                double left = pen_x_measure + g->bitmap_left;
                double right = left + g->bitmap.width;
                double top = g->bitmap_top;
                double bottom = top - g->bitmap.rows;

                if (left < minX)
                {
                    minX = left;
                }
                if (right > maxX)
                {
                    maxX = right;
                }
                if (top > maxY)
                {
                    maxY = top;
                }
                if (bottom < minY)
                {
                    minY = bottom;
                }
            }

            pen_x_measure += (g->advance.x >> 6);
        }

        double text_mid_x = (minX + maxX) * 0.5;
        double text_mid_y = (minY + maxY) * 0.5;

        // ---------------------------------------------------------------------
        // STEP 2: Use instance geometry
        // ---------------------------------------------------------------------
        const auto& g = oled.geometry();
        double screen_cx = g.width * 0.5;
        double screen_cy = g.height * 0.5;

        // Optional aesthetic shift upward
        screen_cy -= 4.0;

        // ---------------------------------------------------------------------
        // STEP 3: Rotation math
        // ---------------------------------------------------------------------
        double rad = -(angle_deg * M_PI / 180.0);
        double cos_a = std::cos(rad);
        double sin_a = std::sin(rad);

        // ---------------------------------------------------------------------
        // PASS 4: Render rotated text
        // ---------------------------------------------------------------------
        const char* p_draw = display_text;
        double pen_x_draw = 0.0;

        while (*p_draw)
        {
            std::uint32_t cp = decode_utf8(p_draw);
            FT_Load_Char(renderer.face, cp, FT_LOAD_RENDER);
            FT_GlyphSlot g = renderer.face->glyph;

            for (int row = 0; row < g->bitmap.rows; ++row)
            {
                for (int col = 0; col < g->bitmap.width; ++col)
                {
                    if (g->bitmap.buffer[row * g->bitmap.pitch + col] > 64)
                    {
                        double lx = pen_x_draw + g->bitmap_left + col;
                        double ly = g->bitmap_top - row;

                        double rx = lx - text_mid_x;
                        double ry = ly - text_mid_y;

                        double rot_x = rx * cos_a - ry * sin_a;
                        double rot_y = rx * sin_a + ry * cos_a;

                        int px = static_cast<int>(screen_cx + rot_x);
                        int py = static_cast<int>(screen_cy - rot_y);

                        oled.drawPixel(px, py, true);
                    }
                }
            }

            pen_x_draw += (g->advance.x >> 6);
        }

        oled.update();

        angle_deg += 15.0;
        if (angle_deg >= 360.0)
        {
            angle_deg = 0.0;
        }

        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

} // namespace muc::fonts
