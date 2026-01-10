#include "ui_queue.h"

#include "esp_log.h"

namespace muc::ui
{

UiQueue::UiQueue(std::size_t depth)
: m_queue(xQueueCreate(depth, sizeof(UiMessage)))
{
}

bool UiQueue::send(const UiMessage& msg) const
{
    ESP_LOGI("UI_QUEUE", "send");
    return xQueueSend(m_queue, &msg, 0) == pdTRUE;
}

bool UiQueue::receive(UiMessage& msg) const
{
    ESP_LOGI("UI_QUEUE", "receive wait");
    bool ok = xQueueReceive(m_queue, &msg, portMAX_DELAY) == pdTRUE;
    if (ok)
    {
        ESP_LOGI("UI_QUEUE", "receive OK");
    }
    return ok;
}

} // namespace muc::ui
