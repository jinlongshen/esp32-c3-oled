#ifndef MAIN_INC_PROVISIONHOOKS_H
#define MAIN_INC_PROVISIONHOOKS_H

#include <atomic>
#include <string_view>

#include "ssd1306.h"
#include "ui_api.h"

namespace muc::provisionhooks
{

extern std::atomic<bool> g_is_provisioned;
extern std::atomic<bool> g_can_fetch_stock;

void on_qr(muc::ui::UiApi& ui_api, muc::ssd1306::Oled& oled, std::string_view qr_payload);

void on_success(muc::ui::UiApi& ui_api, muc::ssd1306::Oled& oled, std::string_view ip_address);

} // namespace muc::provisionhooks

#endif // MAIN_INC_PROVISIONHOOKS_H