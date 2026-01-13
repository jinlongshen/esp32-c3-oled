#ifndef COMPONENTS_PROVISION_INC_PROVISION_H
#define COMPONENTS_PROVISION_INC_PROVISION_H

#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>

#include <esp_err.h>
#include <esp_event.h>
#include <driver/gpio.h>

namespace muc::provision
{

class Provision
{
  public:
    Provision();

    // Core lifecycle: Init Wi-Fi, NVS, and start Provisioning if needed
    esp_err_t begin();

    // Reset provisioning and reboot
    void forceReProvision();

    // Blocks until Wi-Fi is connected
    void wait_for_connection();

  private:
    // Background button monitor (static for FreeRTOS)
    static void button_task(void* pvParameters);

    // Event handler for Wi-Fi and Provisioning (static for ESP-IDF Event Loop)
    static void event_handler(void* arg, esp_event_base_t base, int32_t id, void* data);

    // Internal helper methods
    void start_provisioning();
    static void get_device_service_name(char* service_name, size_t max);

    EventGroupHandle_t _wifi_event_group;
    static constexpr int CONNECTED_BIT = BIT0;
    static constexpr gpio_num_t BOO_BUTTON_GPIO = GPIO_NUM_9;
};

} // namespace muc::provision

#endif // COMPONENTS_PROVISION_INC_PROVISION_H