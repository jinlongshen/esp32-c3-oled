#ifndef COMPONENTS_UI_INC_UI_CONSUMER_TASK_H
#define COMPONENTS_UI_INC_UI_CONSUMER_TASK_H

#include "freertos/FreeRTOS.h"
#include "lvgl.h"
#include "ui_queue.h" // <--- CRITICAL INCLUDE

namespace muc::ui
{

struct LvglTaskConfig
{
    uint32_t tick_period_ms;
    uint32_t handler_period_ms;
    UiQueue* user_data; // <--- MUST BE DEFINED HERE
};

class UiConsumerTask
{
  public:
    static void ui_init_task(void* arg);
    static void lvgl_handler_task(void* arg);
    static void lvgl_tick_task(void* arg);

  private:
    static void set_view_mode(bool provisioning);
    static lv_obj_t* s_counter_label;
    static lv_obj_t* s_status_label;
    static lv_obj_t* s_qr_code;
};

} // namespace muc::ui

#endif // COMPONENTS_UI_INC_UI_CONSUMER_TASK_H