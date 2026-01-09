#include "ui_consumer_task.h"

namespace muc::ui
{

UiConsumerTask::UiConsumerTask(UiQueue& queue, lv_obj_t* label)
: m_queue(queue)
, m_label(label)
{
}

void UiConsumerTask::task_entry(void* arg)
{
    auto* self = static_cast<UiConsumerTask*>(arg);

    UiMessage msg{};

    while (true)
    {
        if (self->m_queue.receive(msg))
        {
            if (msg.type == UiCommandType::SetLabelText)
            {
                lv_label_set_text(self->m_label, msg.text.data());
            }
            else if (msg.type == UiCommandType::ClearScreen)
            {
                lv_obj_clean(lv_scr_act());
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

} // namespace muc::ui
