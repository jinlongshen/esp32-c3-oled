#include "ui_api.h"
#include <cstring>

namespace muc::ui
{

UiApi::UiApi(UiQueue& queue)
: m_queue(queue)
{
}

void UiApi::set_text(std::string_view text)
{
    UiMessage msg{};
    msg.type = UiCommandType::SetLabelText;

    std::strncpy(msg.text.data(), text.data(), msg.text.size());
    m_queue.send(msg);
}

void UiApi::clear_screen()
{
    UiMessage msg{};
    msg.type = UiCommandType::ClearScreen;
    m_queue.send(msg);
}

} // namespace muc::ui
