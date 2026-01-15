#ifndef COMPONENTS_PROVISION_INC_PROVISION_H
#define COMPONENTS_PROVISION_INC_PROVISION_H

#include <cstdint>
#include <functional>
#include <string_view>

#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>

#include <esp_err.h>
#include <esp_event.h>

namespace muc::provision
{

class Provision
{
  public:
    Provision();

    /**
     * @brief Starts the provisioning process.
     * @param on_qr_generated Callback triggered when a QR payload is ready for the UI.
     * @param on_got_ip Callback triggered when the router assigns an IP address.
     */
    esp_err_t begin(std::function<void(std::string_view)> on_qr_generated,
                    std::function<void(std::string_view)> on_got_ip);

    void wait_for_connection();
    void forceReProvision();

  private:
    static constexpr std::uint32_t s_CONNECTED_BIT = BIT0;
    EventGroupHandle_t m_wifi_event_group;

    std::function<void(std::string_view)> m_on_qr_generated;
    std::function<void(std::string_view)> m_on_got_ip;

    void start_provisioning();
    static void event_handler(void* arg, esp_event_base_t base, std::int32_t id, void* data);
    static void button_task(void* pvParameters);
    void get_device_service_name(char* service_name, std::size_t max);
};

} // namespace muc::provision

#endif // COMPONENTS_PROVISION_INC_PROVISION_H