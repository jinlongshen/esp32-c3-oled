#include "ui_consumer_task.h"

#include <esp_log.h>

#include "lvgl.h"
#include "ui_api.h"

namespace muc::ui
{

// Static members
lv_obj_t* UiConsumerTask::s_label = nullptr;
lv_obj_t* UiConsumerTask::s_status_label = nullptr;
lv_obj_t* UiConsumerTask::s_cpu_percent_label = nullptr;

//
// UI INIT TASK
//
void UiConsumerTask::ui_init_task(void* arg)
{
    auto* api = static_cast<UiApi*>(arg);
    configASSERT(api && "ui_init_task: null UiApi");

    lv_coord_t disp_w = lv_display_get_horizontal_resolution(NULL);

    //
    // MAIN LABEL (top, centered)
    //
    lv_obj_t* label = lv_label_create(lv_scr_act());
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(label, disp_w);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(label, "");
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 2);
    UiConsumerTask::set_label(label);

    //
    // STATUS LABEL (bottom-left)
    //
    lv_obj_t* status = lv_label_create(lv_scr_act());
    lv_label_set_text(status, "Rdy");
    lv_obj_align(status, LV_ALIGN_BOTTOM_LEFT, 2, -1);
    UiConsumerTask::set_status_label(status);

    //
    // CPU PERCENT LABEL (bottom-right)
    //
    lv_obj_t* cpu_pct = lv_label_create(lv_scr_act());
    lv_label_set_text(cpu_pct, "00%");
    lv_obj_align(cpu_pct, LV_ALIGN_BOTTOM_RIGHT, -2, -1);
    UiConsumerTask::set_cpu_percent_label(cpu_pct);

    //
    // Initial text
    //
    api->set_text("Demo");

    vTaskDelete(nullptr);
}

//
// SETTERS
//
void UiConsumerTask::set_label(lv_obj_t* lbl)
{
    s_label = lbl;
}

void UiConsumerTask::set_status_label(lv_obj_t* lbl)
{
    s_status_label = lbl;
}

void UiConsumerTask::set_cpu_percent_label(lv_obj_t* lbl)
{
    s_cpu_percent_label = lbl;
}

//
// LVGL TICK TASK
//
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

//
// LVGL HANDLER TASK
//
void UiConsumerTask::lvgl_handler_task(void* arg)
{
    auto* cfg = static_cast<const LvglTaskConfig*>(arg);
    configASSERT(cfg && "lvgl_handler_task: null config");

    auto* queue = static_cast<UiQueue*>(cfg->user_data);
    configASSERT(queue && "lvgl_handler_task: missing UiQueue");

    UiMessage msg{};

    while (true)
    {
        if (queue->receive(msg))
        {
            switch (msg.type)
            {
            case UiCommandType::SetLabelText:
                if (s_label)
                    lv_label_set_text(s_label, msg.text.data());
                break;

            case UiCommandType::SetStatusText:
                if (s_status_label)
                    lv_label_set_text(s_status_label, msg.text.data());
                break;

            case UiCommandType::SetCpuUsage:
            {
                int percent = msg.value;
                if (percent < 0)   percent = 0;
                if (percent > 100) percent = 100;

                if (s_cpu_percent_label)
                {
                    char buf[8];
                    snprintf(buf, sizeof(buf), "%02d%%", percent);
                    lv_label_set_text(s_cpu_percent_label, buf);
                }
                break;
            }

            case UiCommandType::ClearScreen:
                lv_obj_clean(lv_scr_act());
                break;
            }
        }

        lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(cfg->handler_period_ms));
    }
}

} // namespace muc::ui
