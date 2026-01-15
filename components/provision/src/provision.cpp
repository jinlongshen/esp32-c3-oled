#include "provision.h"

#include <cstdint>
#include <cstdio>
#include <cstring>

#include <esp_event.h>
#include <esp_log.h>
#include <esp_wifi.h>
#include <nvs_flash.h>
#include <driver/gpio.h>
#include <wifi_provisioning/manager.h>
#include <wifi_provisioning/scheme_ble.h>

#include "lvgl.h"

namespace muc::provision
{

namespace
{
constexpr const char* TAG = "PROVISION";
constexpr gpio_num_t BOO_BUTTON_GPIO = GPIO_NUM_9;
} // namespace

Provision::Provision()
{
    m_wifi_event_group = xEventGroupCreate();
}

esp_err_t Provision::begin(std::function<void(std::string_view)> on_qr_generated,
                           std::function<void(std::string_view)> on_got_ip)
{
    // Store callbacks to communicate with UI
    m_on_qr_generated = on_qr_generated;
    m_on_got_ip = on_got_ip;

    // 1. Initialize NVS
    auto ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // 2. Network Interface
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    // 3. Register Event Handlers (Using std::int32_t)
    ESP_ERROR_CHECK(esp_event_handler_register(
        WIFI_PROV_EVENT, ESP_EVENT_ANY_ID, &Provision::event_handler, this));
    ESP_ERROR_CHECK(
        esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &Provision::event_handler, this));
    ESP_ERROR_CHECK(
        esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &Provision::event_handler, this));

    // 4. Initialize Wi-Fi
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // 5. Provisioning Manager Config
    wifi_prov_mgr_config_t config = {};
    config.scheme = wifi_prov_scheme_ble;
    config.scheme_event_handler = WIFI_PROV_SCHEME_BLE_EVENT_HANDLER_FREE_BTDM;

    ESP_ERROR_CHECK(wifi_prov_mgr_init(config));

    bool provisioned = false;
    ESP_ERROR_CHECK(wifi_prov_mgr_is_provisioned(&provisioned));

    if (!provisioned)
    {
        start_provisioning();
    }
    else
    {
        ESP_LOGI(TAG, "Already provisioned, starting Wi-Fi station");
        wifi_prov_mgr_deinit();
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
        ESP_ERROR_CHECK(esp_wifi_start());
    }

    // 6. Start button polling task (4 * 1024 stack)
    xTaskCreate(Provision::button_task, "prov_btn_task", 4096, this, 5, nullptr);

    return ESP_OK;
}

void Provision::start_provisioning()
{
    auto service_name = std::array<char, 12>{};
    get_device_service_name(service_name.data(), service_name.size());
    static constexpr const char* pop = "abcd1234";

    ESP_ERROR_CHECK(wifi_prov_mgr_start_provisioning(
        WIFI_PROV_SECURITY_1, (void*)pop, service_name.data(), nullptr));

    auto qr_payload = std::array<char, 150>{};
    std::snprintf(qr_payload.data(),
                  qr_payload.size(),
                  "{\"ver\":\"v1\",\"name\":\"%s\",\"pop\":\"%s\",\"transport\":\"ble\"}",
                  service_name.data(),
                  pop);

    // 1. Notify UI to show QR Code via the callback
    // This sends the payload to main.cpp -> UiApi -> UiConsumerTask (LVGL)
    if (m_on_qr_generated)
    {
        m_on_qr_generated(std::string_view{qr_payload.data()});
    }

    // 2. Console logging (Standard text only)
    ESP_LOGI(TAG, "Provisioning started. Service: %s, POP: %s", service_name.data(), pop);
    ESP_LOGD(TAG, "QR Payload: %s", qr_payload.data());
}

void Provision::event_handler(void* arg, esp_event_base_t base, std::int32_t id, void* data)
{
    auto* self = static_cast<Provision*>(arg);
    configASSERT(self != nullptr && "Provision::event_handler received null parameter");

    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    }
    else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP)
    {
        auto* event = static_cast<ip_event_got_ip_t*>(data);
        auto ip_buf = std::array<char, 16>{};
        esp_ip4addr_ntoa(&event->ip_info.ip, ip_buf.data(), ip_buf.size());

        ESP_LOGI(TAG, "Connected! IP Address: %s", ip_buf.data());

        // Notify UI to hide QR and show the dynamic IP from the router
        if (self->m_on_got_ip)
        {
            self->m_on_got_ip(std::string_view{ip_buf.data()});
        }

        xEventGroupSetBits(self->m_wifi_event_group, s_CONNECTED_BIT);
    }
}

void Provision::get_device_service_name(char* service_name, std::size_t max)
{
    auto eth_mac = std::array<std::uint8_t, 6>{};
    esp_wifi_get_mac(WIFI_IF_STA, eth_mac.data());
    std::snprintf(service_name, max, "PROV_%02X%02X%02X", eth_mac[3], eth_mac[4], eth_mac[5]);
}

void Provision::forceReProvision()
{
    ESP_LOGW(TAG, "Resetting provisioning...");
    wifi_prov_mgr_reset_provisioning();
    vTaskDelay(pdMS_TO_TICKS(100));
    esp_restart();
}

void Provision::button_task(void* pvParameters)
{
    auto* self = static_cast<Provision*>(pvParameters);
    configASSERT(self != nullptr && "Provision::button_task received null parameter");
    auto consecutive_press_count = std::int32_t{0};
    static constexpr auto REQUIRED_COUNT = 4;
    static constexpr auto POLL_PERIOD_MS = 500;

    gpio_config_t io_conf = {};
    io_conf.pin_bit_mask = (1ULL << BOO_BUTTON_GPIO);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&io_conf);

    while (true)
    {
        if (gpio_get_level(BOO_BUTTON_GPIO) == 0)
        {
            if (++consecutive_press_count >= REQUIRED_COUNT)
            {
                self->forceReProvision();
            }
        }
        else
        {
            consecutive_press_count = 0;
        }
        vTaskDelay(pdMS_TO_TICKS(POLL_PERIOD_MS));
    }
}

} // namespace muc::provision