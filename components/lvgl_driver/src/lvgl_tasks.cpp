#include "lvgl_tasks.h"

#include <cstdint>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"

namespace muc::lvgl_driver
{

void lvgl_tick_task(void* arg)
{
    constexpr std::uint32_t period_ms = 10;

    while (true)
    {
        lv_tick_inc(period_ms);
        vTaskDelay(pdMS_TO_TICKS(period_ms));
    }
}

void lvgl_handler_task(void* arg) noexcept
{
    while (true)
    {
        lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

} // namespace muc::lvgl_driver
