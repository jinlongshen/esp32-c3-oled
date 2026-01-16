#include "ui_consumer_task.h"

#include <cstring>

#include <esp_log.h>

#include "lvgl.h"
#include "ui_queue.h"

namespace muc::ui
{

lv_obj_t* UiConsumerTask::s_counter_label = nullptr;
lv_obj_t* UiConsumerTask::s_status_label = nullptr;
lv_obj_t* UiConsumerTask::s_qr_code = nullptr;
lv_obj_t* UiConsumerTask::s_qr_container = nullptr;

void UiConsumerTask::ui_init_task(void* arg)
{
    static lv_style_t style_main;
    lv_style_init(&style_main);
    lv_style_set_text_font(&style_main, &lv_font_montserrat_12);

    // Counter label (top)
    s_counter_label = lv_label_create(lv_scr_act());
    lv_obj_add_style(s_counter_label, &style_main, 0);
    lv_obj_align(s_counter_label, LV_ALIGN_TOP_MID, 0, 0);
    lv_label_set_text(s_counter_label, "0");

    // Status label (bottom)
    s_status_label = lv_label_create(lv_scr_act());
    lv_obj_add_style(s_status_label, &style_main, 0);
    lv_obj_set_width(s_status_label, 72);
    lv_obj_set_style_text_align(s_status_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_long_mode(s_status_label, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_align(s_status_label, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_label_set_text(s_status_label, "START");

    // Hide labels initially (provisioning mode)
    lv_obj_add_flag(s_counter_label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(s_status_label, LV_OBJ_FLAG_HIDDEN);

    vTaskDelete(nullptr);
}

void UiConsumerTask::set_view_mode(bool provisioning)
{
    if (provisioning)
    {
        // Hide labels
        if (s_counter_label)
        {
            lv_obj_add_flag(s_counter_label, LV_OBJ_FLAG_HIDDEN);
        }
        if (s_status_label)
        {
            lv_obj_add_flag(s_status_label, LV_OBJ_FLAG_HIDDEN);
        }

        // Show QR container
        if (s_qr_container)
        {
            lv_obj_clear_flag(s_qr_container, LV_OBJ_FLAG_HIDDEN);
        }
    }
    else
    {
        // Show labels
        if (s_counter_label)
        {
            lv_obj_clear_flag(s_counter_label, LV_OBJ_FLAG_HIDDEN);
        }
        if (s_status_label)
        {
            lv_obj_clear_flag(s_status_label, LV_OBJ_FLAG_HIDDEN);
        }

        // DELETE QR container completely
        if (s_qr_container)
        {
            lv_obj_del(s_qr_container);
            s_qr_container = nullptr;
            s_qr_code = nullptr;
        }
    }
}

void UiConsumerTask::lvgl_handler_task(void* arg)
{
    auto* cfg = static_cast<const LvglTaskConfig*>(arg);
    auto* queue = static_cast<UiQueue*>(cfg->user_data);
    UiMessage msg{};

    while (true)
    {
        if (queue->receive(msg))
        {
            switch (msg.type)
            {
            case UiCommandType::SetText:
                if (s_counter_label)
                {
                    lv_label_set_text(s_counter_label, msg.text.data());
                }
                break;

            case UiCommandType::SetStatus:
                set_view_mode(false);
                if (s_status_label)
                {
                    lv_label_set_text(s_status_label, msg.text.data());
                }
                break;

            case UiCommandType::ShowQrCode:
                if (!s_qr_code)
                {
                    // Create container
                    s_qr_container = lv_obj_create(lv_scr_act());
                    lv_obj_set_size(s_qr_container, 62, 62);
                    lv_obj_set_style_bg_color(s_qr_container, lv_color_white(), 0);
                    lv_obj_set_style_border_width(s_qr_container, 0, 0);
                    lv_obj_set_style_radius(s_qr_container, 0, 0);
                    lv_obj_set_style_pad_all(s_qr_container, 2, 0);
                    lv_obj_center(s_qr_container);

                    // Create QR inside container
                    s_qr_code = lv_qrcode_create(s_qr_container);
                    lv_qrcode_set_size(s_qr_code, 58);
                    lv_qrcode_set_dark_color(s_qr_code, lv_color_black());
                    lv_qrcode_set_light_color(s_qr_code, lv_color_white());
                    lv_obj_center(s_qr_code);
                }

                lv_qrcode_update(s_qr_code, msg.text.data(), strlen(msg.text.data()));
                set_view_mode(true);
                break;

            default:
                break;
            }
        }

        lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(cfg->handler_period_ms));
    }
}

void UiConsumerTask::lvgl_tick_task(void* arg)
{
    auto* cfg = static_cast<const LvglTaskConfig*>(arg);
    while (true)
    {
        lv_tick_inc(cfg->tick_period_ms);
        vTaskDelay(pdMS_TO_TICKS(cfg->tick_period_ms));
    }
}

} // namespace muc::ui
