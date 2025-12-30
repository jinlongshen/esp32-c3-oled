#include <cstdio>
#include <cstring>
#include <cstdint>
#include <cmath>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "ssd1306.h"
#include "FontRenderer.h"

namespace
{
const char* TAG = "FontExample";

// Linker symbols for the embedded font file
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

void font_task(void* pvParameters)
{
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

        // --- PASS 1: COMPUTE GLOBAL BOUNDING BOX ---
        double minX = 1e9, maxX = -1e9, minY = 1e9, maxY = -1e9;
        const char* p_measure = display_text;
        double pen_x_measure = 0;

        while (*p_measure)
        {
            std::uint32_t cp = muc::fonts::decode_utf8(p_measure);
            FT_Load_Char(renderer.face, cp, FT_LOAD_RENDER);
            FT_GlyphSlot g = renderer.face->glyph;

            if (g->bitmap.width > 0 && g->bitmap.rows > 0)
            {
                double char_left = pen_x_measure + g->bitmap_left;
                double char_right = char_left + g->bitmap.width;
                double char_top = g->bitmap_top;
                double char_bottom = char_top - (int)g->bitmap.rows;

                if (char_left < minX)
                {
                    minX = char_left;
                }
                if (char_right > maxX)
                {
                    maxX = char_right;
                }
                if (char_top > maxY)
                {
                    maxY = char_top;
                }
                if (char_bottom < minY)
                {
                    minY = char_bottom;
                }
            }
            pen_x_measure += (g->advance.x >> 6);
        }

        double text_mid_x = (minX + maxX) / 2.0;
        double text_mid_y = (minY + maxY) / 2.0;

        const double VISUAL_Y_ADJUST = 1.0;
        text_mid_y -= VISUAL_Y_ADJUST;

        // --- STEP 2: PHYSICAL SCREEN PLACEMENT ---
        double screen_cx = static_cast<double>(muc::ssd1306::OLED_X_OFFSET) + (73.0 / 2.0);

        // Shifting 12 pixels toward the upper part (smaller Y)
        double vertical_shift = -12.0;
        double screen_cy = (static_cast<double>(muc::ssd1306::OLED_Y_OFFSET) +
                            (static_cast<double>(muc::ssd1306::OLED_HEIGHT) / 2.0)) +
                           vertical_shift;

        // --- STEP 3: ROTATION MATH ---
        double rad = -(angle_deg * M_PI / 180.0);
        double cos_a = std::cos(rad);
        double sin_a = std::sin(rad);

        // --- PASS 4: RENDER ---
        const char* p_draw = display_text;
        double pen_x_draw = 0;

        while (*p_draw)
        {
            std::uint32_t cp = muc::fonts::decode_utf8(p_draw);
            FT_Load_Char(renderer.face, cp, FT_LOAD_RENDER);
            FT_GlyphSlot g = renderer.face->glyph;

            for (int row = 0; row < static_cast<int>(g->bitmap.rows); ++row)
            {
                for (int col = 0; col < static_cast<int>(g->bitmap.width); ++col)
                {
                    if (g->bitmap.buffer[row * g->bitmap.pitch + col] > 64)
                    {
                        double lx = pen_x_draw + g->bitmap_left + col;
                        double ly = g->bitmap_top - row;

                        double rx = lx - text_mid_x;
                        double ry = ly - text_mid_y;

                        double rot_x = rx * cos_a - ry * sin_a;
                        double rot_y = rx * sin_a + ry * cos_a;

                        oled.drawPixel(static_cast<int>(screen_cx + rot_x),
                                       static_cast<int>(screen_cy - rot_y),
                                       true);
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