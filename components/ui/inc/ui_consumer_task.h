#ifndef COMPONENTS_UI_INC_UI_CONSUMER_TASK_H
#define COMPONENTS_UI_INC_UI_CONSUMER_TASK_H

#include "ui_queue.h"
#include "lvgl.h"

namespace muc::ui
{

struct LvglTaskConfig
{
    std::uint32_t tick_period_ms;
    std::uint32_t handler_period_ms;
    UiQueue* user_data;
};

class UiConsumerTask
{
  public:
    static void ui_init_task(void* arg);
    static void lvgl_tick_task(void* arg);
    static void lvgl_handler_task(void* arg);

    static void set_label(lv_obj_t* lbl);

  private:
    static lv_obj_t* s_label;
};

} // namespace muc::ui

#endif // COMPONENTS_UI_INC_UI_CONSUMER_TASK_H
