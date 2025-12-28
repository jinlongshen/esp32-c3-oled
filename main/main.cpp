#include "I2CBus.h"
#include "I2CDevice.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ssd1306.h"

#include <ft2build.h>
#include FT_FREETYPE_H

extern "C" void app_main(void);

// Embedded font (subset with ASCII + umlauts)
extern const uint8_t _binary_oled_subset_ascii_umlaut_ttf_start[] asm(
    "_binary_oled_subset_ascii_umlaut_ttf_start");
extern const uint8_t _binary_oled_subset_ascii_umlaut_ttf_end[] asm(
    "_binary_oled_subset_ascii_umlaut_ttf_end");

// --- UTF‑8 decoder (handles ä ö ü Ä Ö Ü ß) ---
static uint32_t decode_utf8(const char*& p)
{
    uint8_t c = (uint8_t)*p++;

    if (c < 0x80)
        return c;

    if ((c & 0xE0) == 0xC0)
        return ((c & 0x1F) << 6) | ((uint8_t)*p++ & 0x3F);

    if ((c & 0xF0) == 0xE0)
    {
        uint32_t c2 = (uint8_t)*p++;
        uint32_t c3 = (uint8_t)*p++;
        return ((c & 0x0F) << 12) | ((c2 & 0x3F) << 6) | (c3 & 0x3F);
    }

    return '?';
}

// --- Blit FreeType bitmap into your framebuffer ---
static void blitBitmap(muc::ssd1306::Oled& oled, int x, int y, FT_Bitmap& bmp)
{
    for (int row = 0; row < bmp.rows; row++)
    {
        for (int col = 0; col < bmp.width; col++)
        {
            uint8_t v = bmp.buffer[row * bmp.pitch + col];
            if (v > 128)
                oled.drawPixel(x + col, y + row, true);
        }
    }
}

// --- Render UTF‑8 text ---
static void renderText(muc::ssd1306::Oled& oled,
                       FT_Face face,
                       int x,
                       int baselineY,
                       const char* text)
{
    const char* p = text;
    int penX = x;

    while (*p)
    {
        uint32_t cp = decode_utf8(p);

        if (FT_Load_Char(face, cp, FT_LOAD_RENDER))
            continue;

        FT_GlyphSlot g = face->glyph;

        int gx = penX + g->bitmap_left;
        int gy = baselineY - g->bitmap_top;

        blitBitmap(oled, gx, gy, g->bitmap);

        penX += g->advance.x >> 6;
    }
}

extern "C" void app_main(void)
{
    using namespace muc;
    using namespace muc::ssd1306;

    // --- OLED init ---
    I2CBus bus(I2C1_PORT, I2C1_SDA_PIN, I2C1_SCL_PIN);
    I2CDevice dev(bus, OLED_ADDR, I2C1_FREQ);
    Oled oled(dev);

    // --- Known-good tests ---
    oled.drawVisibleBar();
    vTaskDelay(pdMS_TO_TICKS(1000));

    oled.drawCharA(16, 8);
    vTaskDelay(pdMS_TO_TICKS(1000));

    for (int x = OLED_X_OFFSET; x < OLED_X_OFFSET + OLED_WIDTH; ++x)
    {
        oled.drawPixel(x, OLED_Y_OFFSET, true); // top row of visible window
    }
    oled.update();

    // // --- FreeType init ---
    // FT_Library ft;
    // FT_Init_FreeType(&ft);

    // size_t font_size =
    //     _binary_oled_subset_ascii_umlaut_ttf_end -
    //     _binary_oled_subset_ascii_umlaut_ttf_start;

    // FT_Face face;
    // FT_New_Memory_Face(ft,
    //                    _binary_oled_subset_ascii_umlaut_ttf_start,
    //                    font_size,
    //                    0,
    //                    &face);

    // FT_Set_Pixel_Sizes(face, 0, 16);  // 16px fits your 64px height

    // // --- Render German word with umlauts ---
    // oled.clear();

    // const char* german = "Übergröße Ölmaß Fähig";

    // // Baseline at y=28 → safely inside your 64px logical area
    // renderText(oled, face, 0, 28, german);

    while (true)
        vTaskDelay(pdMS_TO_TICKS(1000));
}
