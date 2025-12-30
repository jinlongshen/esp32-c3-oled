#include <cstdio>
#include <cstring>
#include <cstdint>
#include <cmath>

// FreeRTOS & ESP32
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

// Project headers
#include "I2CBus.h"
#include "I2CDevice.h"
#include "ssd1306.h"

// FreeType
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

namespace
{
[[maybe_unused]] const char* TAG = "FontSystem";

extern "C"
{
extern const std::uint8_t _binary_oled_subset_ascii_umlaut_ttf_start[] asm(
    "_binary_oled_subset_ascii_umlaut_ttf_start");
extern const std::uint8_t _binary_oled_subset_ascii_umlaut_ttf_end[] asm(
    "_binary_oled_subset_ascii_umlaut_ttf_end");
}

std::uint32_t decode_utf8(const char*& p)
{
    std::uint8_t c = static_cast<std::uint8_t>(*p++);
    if (c < 0x80)
        return c;
    if ((c & 0xE0) == 0xC0)
        return ((c & 0x1F) << 6) | (static_cast<std::uint8_t>(*p++) & 0x3F);
    if ((c & 0xF0) == 0xE0)
    {
        std::uint8_t c2 = static_cast<std::uint8_t>(*p++);
        std::uint8_t c3 = static_cast<std::uint8_t>(*p++);
        return ((c & 0x0F) << 12) | ((c2 & 0x3F) << 6) | (c3 & 0x3F);
    }
    return '?';
}

struct FontRenderer
{
    FT_Library library{nullptr};
    FT_Face face{nullptr};

    bool init(const std::uint8_t* data, std::size_t size, int pixel_size)
    {
        if (FT_Init_FreeType(&library))
            return false;
        if (FT_New_Memory_Face(
                library, data, static_cast<FT_Long>(size), 0, &face))
            return false;
        FT_Set_Pixel_Sizes(face, 0, pixel_size);
        return true;
    }

    ~FontRenderer()
    {
        if (face)
            FT_Done_Face(face);
        if (library)
            FT_Done_FreeType(library);
    }
};
} // namespace

void font_task(void* pvParameters)
{
    auto& oled = *static_cast<muc::ssd1306::Oled*>(pvParameters);
    FontRenderer renderer;

    std::size_t font_size = reinterpret_cast<std::uintptr_t>(
                                _binary_oled_subset_ascii_umlaut_ttf_end) -
                            reinterpret_cast<std::uintptr_t>(
                                _binary_oled_subset_ascii_umlaut_ttf_start);

    // Font size 18px as requested
    if (!renderer.init(
            _binary_oled_subset_ascii_umlaut_ttf_start, font_size, 18))
    {
        vTaskDelete(nullptr);
    }

    double angle_deg = 0.0;
    const char* display_text = "Grüße";

    while (true)
    {
        oled.clear();

        // --- PASS 1: COMPUTE GLOBAL BOUNDING BOX (The "Axle" calculation) ---
        double minX = 1e9, maxX = -1e9, minY = 1e9, maxY = -1e9;
        const char* p_measure = display_text;
        double pen_x_measure = 0;

        while (*p_measure)
        {
            std::uint32_t cp = decode_utf8(p_measure);
            FT_Load_Char(renderer.face, cp, FT_LOAD_RENDER);
            FT_GlyphSlot g = renderer.face->glyph;

            if (g->bitmap.width > 0 && g->bitmap.rows > 0)
            {
                double char_left = pen_x_measure + g->bitmap_left;
                double char_right = char_left + g->bitmap.width;
                double char_top = g->bitmap_top;
                double char_bottom = char_top - (int)g->bitmap.rows;

                if (char_left < minX)
                    minX = char_left;
                if (char_right > maxX)
                    maxX = char_right;
                if (char_top > maxY)
                    maxY = char_top;
                if (char_bottom < minY)
                    minY = char_bottom;
            }
            pen_x_measure += (g->advance.x >> 6);
        }

        // Mathematical center of the pixels (The Pivot)
        double text_mid_x = (minX + maxX) / 2.0;
        double text_mid_y = (minY + maxY) / 2.0;

        // KEEP THIS SMALL (0.0 to 2.0) to prevent "orbiting"
        // This just balances the "Ü" dots vs the "U" body
        const double VISUAL_Y_ADJUST = 1.0;
        text_mid_y -= VISUAL_Y_ADJUST;

        // --- STEP 2: PHYSICAL SCREEN PLACEMENT ---
        double screen_cx =
            static_cast<double>(muc::ssd1306::OLED_X_OFFSET) + (73.0 / 2.0);

        // ADJUST THIS to move the entire animation UP or DOWN
        // Subtract more (e.g., -6.0) to move the whole spinning text HIGHER
        double vertical_nudge = -5.0;
        double screen_cy =
            (static_cast<double>(muc::ssd1306::OLED_Y_OFFSET) +
             (static_cast<double>(muc::ssd1306::OLED_HEIGHT) / 2.0)) +
            vertical_nudge;

        // --- STEP 3: ROTATION MATH ---
        double rad = -(angle_deg * M_PI / 180.0);
        double cos_a = std::cos(rad);
        double sin_a = std::sin(rad);

        // --- PASS 4: RENDER ---
        const char* p_draw = display_text;
        double pen_x_draw = 0;

        while (*p_draw)
        {
            std::uint32_t cp = decode_utf8(p_draw);
            FT_Load_Char(renderer.face, cp, FT_LOAD_RENDER);
            FT_GlyphSlot g = renderer.face->glyph;

            for (int row = 0; row < static_cast<int>(g->bitmap.rows); ++row)
            {
                for (int col = 0; col < static_cast<int>(g->bitmap.width);
                     ++col)
                {
                    if (g->bitmap.buffer[row * g->bitmap.pitch + col] > 64)
                    {

                        double lx = pen_x_draw + g->bitmap_left + col;
                        double ly = g->bitmap_top - row;

                        // rx/ry is the distance from the pixel to the Axle (text_mid)
                        double rx = lx - text_mid_x;
                        double ry = ly - text_mid_y;

                        // Rotate the pixel around the Axle
                        double rot_x = rx * cos_a - ry * sin_a;
                        double rot_y = rx * sin_a + ry * cos_a;

                        // Map the Axle to the Screen Center (screen_cx/cy)
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
            angle_deg = 0.0;
        vTaskDelay(pdMS_TO_TICKS(50)); // Faster delay for smoother testing
    }
}

extern "C" void app_main(void)
{
    static muc::I2CBus bus(
        muc::I2C1_PORT, muc::I2C1_SDA_PIN, muc::I2C1_SCL_PIN);
    static muc::I2CDevice dev(bus, muc::OLED_ADDR, muc::I2C1_FREQ);
    static muc::ssd1306::Oled oled(dev);
    xTaskCreate(font_task, "font_task", 20480, &oled, 5, nullptr);
}