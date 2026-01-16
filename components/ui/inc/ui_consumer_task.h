#ifndef COMPONENTS_UI_INC_UI_CONSUMER_TASK_H
#define COMPONENTS_UI_INC_UI_CONSUMER_TASK_H

#include <cstdint>

#include "lvgl.h"
#include "ui_queue.h"

namespace muc::ui
{

struct LvglTaskConfig
{
    std::uint32_t tick_period_ms;
    std::uint32_t handler_period_ms;
    void* user_data;
};

class UiConsumerTask
{
  public:
    static void ui_init_task(void* arg);
    static void lvgl_handler_task(void* arg);
    static void lvgl_tick_task(void* arg);

  private:
    static void set_view_mode(bool provisioning);

    // LVGL object pointers
    static lv_obj_t* s_counter_label;
    static lv_obj_t* s_status_label;
    static lv_obj_t* s_qr_code;
    static lv_obj_t* s_qr_container;
};

} // namespace muc::ui

#endif // COMPONENTS_UI_INC_UI_CONSUMER_TASK_H
