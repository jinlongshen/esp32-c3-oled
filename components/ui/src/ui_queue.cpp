#include "ui_queue.h"
#include <algorithm>
#include <cstring>
#include "ui_api.h"

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

    // Manual copy instead of .set_payload()
    size_t len = std::min(text.length(), msg.text.size() - 1);
    std::memcpy(msg.text.data(), text.data(), len);
    msg.text[len] = '\0';

    m_queue.send(msg);
}

void UiApi::set_status(std::string_view status)
{
    UiMessage msg;
    msg.type = UiCommandType::SetStatus;

    size_t len = std::min(status.length(), msg.text.size() - 1);
    std::memcpy(msg.text.data(), status.data(), len);
    msg.text[len] = '\0';

    m_queue.send(msg);
}

void UiApi::show_provision_qr(std::string_view payload)
{
    UiMessage msg;
    msg.type = UiCommandType::ShowQrCode;

    size_t len = std::min(payload.length(), msg.text.size() - 1);
    std::memcpy(msg.text.data(), payload.data(), len);
    msg.text[len] = '\0';

    m_queue.send(msg);
}

} // namespace muc::ui