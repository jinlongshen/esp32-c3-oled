#ifndef COMPONENTS_UI_INC_UI_QUEUE_H
#define COMPONENTS_UI_INC_UI_QUEUE_H

#include <algorithm>
#include <array>
#include <cstring>
#include <string_view>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

namespace muc::ui
{

enum class UiCommandType
{
    SetText,
    SetStatus,
    ShowQrCode
};

struct UiMessage
{
    UiCommandType type;
    std::array<char, 128> text;

    void set_payload(std::string_view sv)
    {
        size_t len = std::min(sv.length(), text.size() - 1);
        std::memcpy(text.data(), sv.data(), len);
        text[len] = '\0';
    }
};

class UiQueue
{
  public:
    explicit UiQueue(size_t queue_size)
    {
        m_handle = xQueueCreate(queue_size, sizeof(UiMessage));
    }

    bool send(const UiMessage& msg)
    {
        return xQueueSend(m_handle, &msg, 0) == pdTRUE;
    }

    bool receive(UiMessage& msg)
    {
        return xQueueReceive(m_handle, &msg, portMAX_DELAY) == pdTRUE;
    }

  private:
    QueueHandle_t m_handle;
};

} // namespace muc::ui

#endif // COMPONENTS_UI_INC_UI_QUEUE_H