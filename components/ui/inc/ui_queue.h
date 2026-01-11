#ifndef COMPONENTS_UI_INC_UI_QUEUE_H
#define COMPONENTS_UI_INC_UI_QUEUE_H

#include <array>
#include <cstdint>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

namespace muc::ui
{

// -----------------------------------------------------------------------------
// Updated command types
// -----------------------------------------------------------------------------
enum class UiCommandType : std::uint8_t
{
    SetLabelText,
    SetStatusText,
    SetCpuUsage,
    ClearScreen,
};

// -----------------------------------------------------------------------------
// Updated message struct
// -----------------------------------------------------------------------------
struct UiMessage
{
    UiCommandType type;

    // Text payload (for label + status)
    std::array<char, 64> text{};

    // Numeric payload (for CPU usage)
    int value = 0;
};

// -----------------------------------------------------------------------------
// Queue wrapper
// -----------------------------------------------------------------------------
class UiQueue
{
  public:
    explicit UiQueue(std::size_t depth = 10);

    bool send(const UiMessage& msg) const;
    bool receive(UiMessage& msg) const;

  private:
    QueueHandle_t m_queue;
};

} // namespace muc::ui

#endif // COMPONENTS_UI_INC_UI_QUEUE_H
