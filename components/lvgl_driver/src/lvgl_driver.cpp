#include "lvgl_driver.h"
#include "ssd1306.h"

#include <array>
#include <cstdint>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

using namespace muc::ssd1306;

extern Oled oled;

namespace muc::lvgl_driver
{

void lvgl_tick_task(void* pvArgument) noexcept
{
    (void)pvArgument;

    while (true)
    {
        lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

// LVGL 9.x flush callback
static void flush_cb(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map)
{
    // Full-screen monochrome buffer, packed according to LV_COLOR_DEPTH
    constexpr uint32_t W = OLED_WIDTH;
    constexpr uint32_t H = OLED_HEIGHT;
    constexpr uint32_t BUF_SIZE = (W * H * LV_COLOR_DEPTH + 7) / 8;

    oled.blitLVGLBuffer(std::span<const uint8_t>(px_map, BUF_SIZE));

    oled.update();

    lv_display_flush_ready(disp);
}

void init() noexcept
{
    lv_init();

    // Assume OLED hardware is initialized elsewhere

    // ---------------------------------
    // Create LVGL display
    // ---------------------------------
    lv_display_t* disp = lv_display_create(OLED_WIDTH, OLED_HEIGHT);

    // ---------------------------------
    // Allocate draw buffer
    // ---------------------------------
    static lv_draw_buf_t draw_buf;

    constexpr uint32_t W = OLED_WIDTH;
    constexpr uint32_t H = OLED_HEIGHT;

    // STRIDE is in pixels; LVGL derives bytes from LV_COLOR_DEPTH
    constexpr uint32_t STRIDE = W;

    // Buffer size in bytes for the chosen color depth
    constexpr uint32_t BUF_SIZE = (W * H * LV_COLOR_DEPTH + 7) / 8;

    static std::array<uint8_t, BUF_SIZE> buf1;

    // lv_draw_buf_init(draw_buf, w, h, cf, stride, buf, buf_size)
    lv_draw_buf_init(&draw_buf, W, H, LV_COLOR_FORMAT_NATIVE, STRIDE, buf1.data(), BUF_SIZE);

    // Attach draw buffer to display (single buffering)
    lv_display_set_draw_buffers(disp, &draw_buf, nullptr);

    // Register flush callback
    lv_display_set_flush_cb(disp, flush_cb);
}

} // namespace muc::lvgl_driver
