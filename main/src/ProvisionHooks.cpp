#include "ProvisionHooks.h"

namespace muc::provisionhooks
{

std::atomic<bool> g_is_provisioned{false};
std::atomic<bool> g_can_fetch_stock{false};

void on_qr(muc::ui::UiApi& ui_api, muc::ssd1306::Oled& oled, std::string_view qr_payload)
{
    oled.set_scan_mode(true);
    ui_api.show_provision_qr(qr_payload);
    g_can_fetch_stock.store(false);
    g_is_provisioned.store(false);
}

void on_success(muc::ui::UiApi& ui_api, muc::ssd1306::Oled& oled, std::string_view ip_address)
{
    oled.set_scan_mode(false);
    ui_api.set_status(ip_address);

    g_is_provisioned.store(true);
    g_can_fetch_stock.store(true);
}

} // namespace muc::provisionhooks
