#ifndef COMPONENTS_UI_INC_UI_CONSUMER_TASK_H
#define COMPONENTS_UI_INC_UI_CONSUMER_TASK_H

#include "ui_queue.h"
#include "lvgl.h"

namespace muc::ui
{

class UiConsumerTask
{
  public:
    UiConsumerTask(UiQueue& queue, lv_obj_t* label);

    static void task_entry(void* arg);

  private:
    UiQueue& m_queue;
    lv_obj_t* m_label;
};

} // namespace muc::ui

#endif // COMPONENTS_UI_INC_UI_CONSUMER_TASK_H
