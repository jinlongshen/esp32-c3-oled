#include "ui_consumer_task.h"

#include "esp_log.h"
#include "lvgl.h"
#include "ui_api.h"

namespace muc::ui
{

// Static storage for the label
lv_obj_t* UiConsumerTask::s_label = nullptr;

void UiConsumerTask::set_label(lv_obj_t* lbl)
{
    s_label = lbl;
}

void UiConsumerTask::lvgl_tick_task(void* arg)
{
    auto* cfg = static_cast<const LvglTaskConfig*>(arg);
    configASSERT(cfg && "lvgl_tick_task: null config");

    while (true)
    {
        lv_tick_inc(cfg->tick_period_ms);
        vTaskDelay(pdMS_TO_TICKS(cfg->tick_period_ms));
    }
}

void UiConsumerTask::lvgl_handler_task(void* arg)
{
    auto* cfg = static_cast<const LvglTaskConfig*>(arg);
    configASSERT(cfg && "lvgl_handler_task: null config");

    // The queue pointer is passed via cfg->user_data or directly via arg
    auto* queue = static_cast<UiQueue*>(cfg->user_data);
    configASSERT(queue && "lvgl_handler_task: missing UiQueue");

    UiMessage msg{};

    while (true)
    {
        // Process UI messages
        if (queue->receive(msg))
        {
            switch (msg.type)
            {
            case UiCommandType::SetLabelText:
                if (s_label)
                {
                    lv_label_set_text(s_label, msg.text.data());
                }
                break;

            case UiCommandType::ClearScreen:
                lv_obj_clean(lv_scr_act());
                break;
            }
        }

        // Run LVGL timers + rendering
        lv_timer_handler();

        vTaskDelay(pdMS_TO_TICKS(cfg->handler_period_ms));
    }
}

void UiConsumerTask::ui_init_task(void* arg)
{
    auto* api = static_cast<UiApi*>(arg);
    configASSERT(api && "ui_init_task: null UiApi");

    // Create label on the active screen
    lv_obj_t* label = lv_label_create(lv_screen_active());

    // Text behavior: wrap long text within display width
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);

    // Query LVGL for display size (no need for display_geometry.h)
    lv_coord_t disp_w = lv_display_get_horizontal_resolution(NULL);
    // Set label width to full display width (no hard-coded 72)
    lv_obj_set_width(label, disp_w);

    // Prevent LVGL from re-centering the label on text updates
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);

    // Initial empty text
    lv_label_set_text(label, "");

    // Place label at center (or change to TOP_RIGHT etc.)
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

    // Make label available to LVGL handler task
    UiConsumerTask::set_label(label);

    // Initial text (will be overwritten by your counter later)
    api->set_text("Demo");

    vTaskDelete(nullptr);
}

} // namespace muc::ui
