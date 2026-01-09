#include "lvgl_tasks.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"

namespace muc::lvgl_driver
{

void lvgl_tick_task(void* arg)
{
    if (arg == nullptr)
    {
        // Fail fast: invalid configuration
        configASSERT(false && "lvgl_tick_task: arg is nullptr");
        vTaskDelete(nullptr);
    }

    auto* cfg = static_cast<const LvglTaskConfig*>(arg);

    while (true)
    {
        lv_tick_inc(cfg->tick_period_ms);
        vTaskDelay(pdMS_TO_TICKS(cfg->tick_period_ms));
    }
}

void lvgl_handler_task(void* arg)
{
    if (arg == nullptr)
    {
        // Fail fast: invalid configuration
        configASSERT(false && "lvgl_handler_task: arg is nullptr");
        vTaskDelete(nullptr);
    }

    auto* cfg = static_cast<const LvglTaskConfig*>(arg);

    while (true)
    {
        lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(cfg->handler_period_ms));
    }
}

} // namespace muc::lvgl_driver
