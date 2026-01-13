#include "ui_api.h"
#include "ui_queue.h"

namespace muc::ui
{

UiApi::UiApi(UiQueue& queue)
: m_queue(queue)
{
}

void UiApi::set_text(std::string_view text)
{
    UiMessage msg;
    msg.type = UiCommandType::SetText;
    msg.set_payload(text); // Helper in UiMessage to copy string safely
    m_queue.send(msg);
}

void UiApi::set_status(std::string_view status)
{
    UiMessage msg;
    msg.type = UiCommandType::SetStatus;
    msg.set_payload(status);
    m_queue.send(msg);
}

void UiApi::show_provision_qr(std::string_view payload)
{
    UiMessage msg;
    msg.type = UiCommandType::ShowQrCode;
    msg.set_payload(payload);
    m_queue.send(msg);
}

} // namespace muc::ui