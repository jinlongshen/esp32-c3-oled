#include "ui_queue.h"

namespace muc::ui
{

UiQueue::UiQueue(std::size_t depth)
: m_queue(xQueueCreate(depth, sizeof(UiMessage)))
{
}

bool UiQueue::send(const UiMessage& msg) const
{
    return xQueueSend(m_queue, &msg, 0) == pdTRUE;
}

bool UiQueue::receive(UiMessage& msg) const
{
    return xQueueReceive(m_queue, &msg, 0) == pdTRUE;
}

} // namespace muc::ui
