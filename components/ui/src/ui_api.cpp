#include "ui_api.h"

#include <algorithm>
#include <cstring>
#include "esp_log.h"

namespace muc::ui
{

UiApi::UiApi(UiQueue& queue)
: m_queue(queue)
{
}

void UiApi::set_text(std::string_view text)
{
    ESP_LOGI("UI_API", "set_text: '%.*s'", int(text.size()), text.data());

    UiMessage msg{};
    msg.type = UiCommandType::SetLabelText;

    const size_t n = std::min(text.size(), sizeof(msg.text) - 1);
    std::memcpy(msg.text.data(), text.data(), n);
    msg.text[n] = '\0';

    if (!m_queue.send(msg))
        ESP_LOGE("UI_API", "Queue send FAILED");
}

void UiApi::set_status(std::string_view text)
{
    ESP_LOGI("UI_API", "set_status: '%.*s'", int(text.size()), text.data());

    UiMessage msg{};
    msg.type = UiCommandType::SetStatusText;

    const size_t n = std::min(text.size(), sizeof(msg.text) - 1);
    std::memcpy(msg.text.data(), text.data(), n);
    msg.text[n] = '\0';

    if (!m_queue.send(msg))
        ESP_LOGE("UI_API", "Queue send FAILED");
}

void UiApi::set_cpu_usage(int percent)
{
    ESP_LOGI("UI_API", "set_cpu_usage: %d", percent);

    UiMessage msg{};
    msg.type = UiCommandType::SetCpuUsage;
    msg.value = percent;

    if (!m_queue.send(msg))
        ESP_LOGE("UI_API", "Queue send FAILED");
}

void UiApi::clear_screen()
{
    UiMessage msg{};
    msg.type = UiCommandType::ClearScreen;

    if (!m_queue.send(msg))
        ESP_LOGE("UI_API", "Queue send FAILED");
}

} // namespace muc::ui
