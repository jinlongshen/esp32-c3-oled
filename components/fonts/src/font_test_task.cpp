#include <cstdint>
#include <cstdio>
#include <cstring>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "ssd1306.h"
#include "FontRenderer.h"

namespace
{
const char* TAG = "FontTest";

// Embedded ASCII-only font
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

static void draw_text(muc::ssd1306::Oled& oled,
                      FontRenderer& renderer,
                      int x,
                      int y,
                      const char* text)
{
    const char* p = text;
    int pen_x = x;

    while (*p)
    {
        std::uint32_t cp = decode_utf8(p);
        FT_Load_Char(renderer.face, cp, FT_LOAD_RENDER);
        FT_GlyphSlot g = renderer.face->glyph;

        for (int row = 0; row < g->bitmap.rows; ++row)
        {
            for (int col = 0; col < g->bitmap.width; ++col)
            {
                if (g->bitmap.buffer[row * g->bitmap.pitch + col] > 64)
                {
                    oled.drawPixel(pen_x + g->bitmap_left + col, y + g->bitmap_top - row, true);
                }
            }
        }

        pen_x += (g->advance.x >> 6);
    }
}

void font_test_task(void* pvParameters)
{
    auto& oled = *static_cast<muc::ssd1306::Oled*>(pvParameters);

    FontRenderer renderer;

    const std::size_t font_size =
        reinterpret_cast<std::uintptr_t>(_binary_oled_subset_ascii_umlaut_ttf_end) -
        reinterpret_cast<std::uintptr_t>(_binary_oled_subset_ascii_umlaut_ttf_start);

    if (!renderer.init(_binary_oled_subset_ascii_umlaut_ttf_start, font_size, 10))
    {
        ESP_LOGE(TAG, "Font init failed");
        vTaskDelete(nullptr);
    }

    while (true)
    {
        oled.clear();

        // ---------------------------------------------------------------------
        // 1. Draw border box around the visible 72×40 window
        // ---------------------------------------------------------------------
        for (int x = 0; x < muc::ssd1306::OLED_WIDTH; ++x)
        {
            oled.drawPixel(x, 0, true);
            oled.drawPixel(x, muc::ssd1306::OLED_HEIGHT - 1, true);
        }

        for (int y = 0; y < muc::ssd1306::OLED_HEIGHT; ++y)
        {
            oled.drawPixel(0, y, true);
            oled.drawPixel(muc::ssd1306::OLED_WIDTH - 1, y, true);
        }

        // ---------------------------------------------------------------------
        // 2. Horizontal ruler (0–71)
        // ---------------------------------------------------------------------
        for (int x = 0; x < muc::ssd1306::OLED_WIDTH; ++x)
        {
            if (x % 4 == 0)
                oled.drawPixel(x, 10, true);
        }

        for (int x = 0; x < muc::ssd1306::OLED_WIDTH; x += 8)
        {
            char buf[8];
            std::snprintf(buf, sizeof(buf), "%d", x);
            draw_text(oled, renderer, x, 12, buf);
        }

        // ---------------------------------------------------------------------
        // 3. Vertical ruler (0–39)
        // ---------------------------------------------------------------------
        for (int y = 0; y < muc::ssd1306::OLED_HEIGHT; ++y)
        {
            if (y % 4 == 0)
                oled.drawPixel(10, y, true);
        }

        for (int y = 0; y < muc::ssd1306::OLED_HEIGHT; y += 8)
        {
            char buf[8];
            std::snprintf(buf, sizeof(buf), "%d", y);
            draw_text(oled, renderer, 12, y, buf);
        }

        // ---------------------------------------------------------------------
        // 4. Print offsets for debugging
        // ---------------------------------------------------------------------
        char info[64];
        std::snprintf(info,
                      sizeof(info),
                      "X=%d Y=%d W=%d H=%d",
                      (int)muc::ssd1306::OLED_X_OFFSET,
                      (int)muc::ssd1306::OLED_Y_OFFSET,
                      (int)muc::ssd1306::OLED_WIDTH,
                      (int)muc::ssd1306::OLED_HEIGHT);

        draw_text(oled, renderer, 2, muc::ssd1306::OLED_HEIGHT - 10, info);

        // ---------------------------------------------------------------------
        // 5. Update display
        // ---------------------------------------------------------------------
        oled.update();

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

} // namespace muc::fonts
