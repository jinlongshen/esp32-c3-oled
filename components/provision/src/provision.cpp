#include "provision.h"

#include <array>
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
    m_on_qr_generated = on_qr_generated;
    m_on_got_ip = on_got_ip;

    auto ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_netif_init());

    ret = esp_event_loop_create_default();
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE)
    {
        ESP_ERROR_CHECK(ret);
    }

    if (esp_netif_get_handle_from_ifkey("WIFI_STA_DEF") == nullptr)
    {
        esp_netif_create_default_wifi_sta();
    }

    ESP_ERROR_CHECK(esp_event_handler_register(
        WIFI_PROV_EVENT, ESP_EVENT_ANY_ID, &Provision::event_handler, this));
    ESP_ERROR_CHECK(
        esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &Provision::event_handler, this));
    ESP_ERROR_CHECK(
        esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &Provision::event_handler, this));

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_prov_mgr_config_t config = {};
    config.scheme = wifi_prov_scheme_ble;
    config.scheme_event_handler = WIFI_PROV_SCHEME_BLE_EVENT_HANDLER_FREE_BTDM;

    ret = wifi_prov_mgr_init(config);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE)
    {
        ESP_ERROR_CHECK(ret);
    }

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

    xTaskCreate(Provision::button_task, "prov_btn_task", 4096, this, 5, nullptr);

    return ESP_OK;
}

void Provision::start_provisioning()
{
    auto service_name = std::array<char, 12>{};
    get_device_service_name(service_name.data(), service_name.size());

    // Security 2 Credentials (from your working diff)
    static constexpr std::array<std::uint8_t, 16> salt = {0x6a,
                                                          0xe8,
                                                          0x25,
                                                          0xdb,
                                                          0x66,
                                                          0xab,
                                                          0x46,
                                                          0xc7,
                                                          0xff,
                                                          0x73,
                                                          0x72,
                                                          0x30,
                                                          0xa8,
                                                          0x2d,
                                                          0x2e,
                                                          0xf4};

    static constexpr std::array<std::uint8_t, 384> verifier = {
        0xd9, 0x35, 0x45, 0xed, 0xb3, 0x0d, 0xdb, 0x26, 0xdb, 0x73, 0x6a, 0xc0, 0xb7, 0xff, 0x15,
        0x2e, 0xd7, 0xb9, 0xbc, 0x1e, 0x7d, 0x98, 0x7f, 0xbd, 0x78, 0x66, 0x6b, 0x62, 0x4d, 0x69,
        0xe4, 0x66, 0xed, 0xdb, 0x94, 0xc2, 0xd1, 0xec, 0xe1, 0xb8, 0xa2, 0xf9, 0x40, 0x42, 0xf2,
        0xe0, 0x82, 0xfe, 0x31, 0x65, 0x13, 0x10, 0x31, 0xc4, 0x9e, 0x91, 0x62, 0xbb, 0xbc, 0xfe,
        0x15, 0xb5, 0xdb, 0x84, 0x9a, 0x06, 0x5e, 0xa7, 0x02, 0x8c, 0x4f, 0x53, 0xd8, 0xef, 0x35,
        0xfa, 0x2e, 0x74, 0xb7, 0xbd, 0xff, 0xac, 0x00, 0xe4, 0x96, 0x2d, 0x1c, 0x3d, 0x04, 0x85,
        0x1f, 0x64, 0xd0, 0x71, 0x61, 0xea, 0x8f, 0x9d, 0x71, 0xee, 0x94, 0x53, 0x76, 0x46, 0xd2,
        0xfd, 0x56, 0x95, 0x60, 0x01, 0x6f, 0xe1, 0xb3, 0x6e, 0x50, 0x47, 0x69, 0x1e, 0xf1, 0x05,
        0x7d, 0xbf, 0x54, 0x25, 0x66, 0xcc, 0xfb, 0xf1, 0x22, 0x0f, 0xcb, 0x86, 0xd4, 0xc8, 0x09,
        0x15, 0x04, 0xf2, 0x33, 0xb3, 0xfa, 0x39, 0xec, 0x20, 0x5b, 0x60, 0xa8, 0x42, 0x89, 0x17,
        0x58, 0x78, 0x2f, 0xdc, 0x68, 0x13, 0x0c, 0xa5, 0x85, 0xda, 0x1a, 0x59, 0x81, 0x3d, 0x01,
        0xf6, 0x6f, 0x56, 0x18, 0x68, 0xdf, 0xd3, 0x36, 0xd2, 0xa3, 0xda, 0x62, 0xb5, 0x5f, 0xd2,
        0x89, 0x53, 0x9c, 0x65, 0x8a, 0x94, 0xa9, 0x2c, 0xb1, 0x75, 0xee, 0x8c, 0xe1, 0x18, 0xbd,
        0xd3, 0xdc, 0xb4, 0xc4, 0xa8, 0x83, 0xd2, 0x6f, 0x72, 0x7e, 0xf4, 0xef, 0x87, 0xa8, 0x42,
        0x89, 0x2b, 0x30, 0xa3, 0xef, 0x31, 0xce, 0x3a, 0x37, 0x64, 0x29, 0x9b, 0xc3, 0xf9, 0x1d,
        0x1e, 0x8c, 0xcc, 0x16, 0xbf, 0x3e, 0x12, 0x10, 0xdc, 0x56, 0xa0, 0xaa, 0xe8, 0xe2, 0x42,
        0xe9, 0xcd, 0xda, 0x54, 0x64, 0x29, 0xd5, 0xec, 0x83, 0x32, 0xf3, 0xb1, 0x93, 0xc4, 0xd3,
        0xa3, 0x6e, 0xa1, 0x04, 0x35, 0x72, 0xe4, 0x82, 0x20, 0x6f, 0x2b, 0x94, 0x2c, 0xba, 0x2c,
        0x6e, 0x6e, 0x39, 0x34, 0xa7, 0x90, 0xc3, 0x3b, 0xa6, 0x99, 0x75, 0xb7, 0xb5, 0xc4, 0x6e,
        0x6d, 0x09, 0x2a, 0x4e, 0x47, 0xfe, 0xd0, 0xed, 0x1f, 0x7e, 0x26, 0xe1, 0xa6, 0x0a, 0x11,
        0x0e, 0x54, 0x49, 0x01, 0xe1, 0xc8, 0x61, 0xed, 0x09, 0xe0, 0x7e, 0xaa, 0x26, 0xf1, 0x14,
        0x82, 0x20, 0x50, 0xb8, 0x01, 0xfa, 0x5d, 0xa4, 0x6e, 0x00, 0xa5, 0xa9, 0x12, 0xf3, 0x3f,
        0x69, 0x38, 0x96, 0x82, 0xaf, 0x9f, 0x83, 0x66, 0xed, 0xd3, 0xe9, 0x14, 0xe6, 0xe2, 0xe6,
        0x40, 0x2d, 0xc8, 0x50, 0xa9, 0xa5, 0x89, 0xd6, 0x32, 0x3c, 0x1c, 0x12, 0xa1, 0x5e, 0x18,
        0x8e, 0xa5, 0xe7, 0xf5, 0xe7, 0x5c, 0x25, 0x3d, 0xa2, 0xfb, 0x5f, 0x30, 0x7e, 0x12, 0x31,
        0xcb, 0x1a, 0x20, 0xfa, 0x3c, 0xe2, 0xf4, 0xdd, 0x98};

    wifi_prov_security2_params_t sec2_params = {
        .salt = reinterpret_cast<const char*>(salt.data()),
        .salt_len = static_cast<uint16_t>(salt.size()),
        .verifier = reinterpret_cast<const char*>(verifier.data()),
        .verifier_len = static_cast<uint16_t>(verifier.size())};

    ESP_ERROR_CHECK(wifi_prov_mgr_start_provisioning(
        WIFI_PROV_SECURITY_2, &sec2_params, service_name.data(), nullptr));

    // QR payload: exactly like your working diff (no "security" field)
    static constexpr const char* pop = "abcd1234";
    auto qr_payload = std::array<char, 150>{};
    std::snprintf(qr_payload.data(),
                  qr_payload.size(),
                  "{\"ver\":\"v1\",\"name\":\"%s\",\"pop\":\"%s\",\"transport\":\"ble\"}",
                  service_name.data(),
                  pop);

    if (m_on_qr_generated)
    {
        m_on_qr_generated(std::string_view{qr_payload.data()});
    }

    ESP_LOGI(TAG, "QR Payload: %s", qr_payload.data());
}

void Provision::event_handler(void* arg, esp_event_base_t base, std::int32_t id, void* data)
{
    auto* self = static_cast<Provision*>(arg);

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

        if (self->m_on_got_ip)
        {
            self->m_on_got_ip(std::string_view{ip_buf.data()});
        }

        xEventGroupSetBits(self->m_wifi_event_group, s_CONNECTED_BIT);
    }
    else if (base == WIFI_PROV_EVENT)
    {
        switch (id)
        {
        case WIFI_PROV_CRED_SUCCESS:
            ESP_LOGI(TAG, "Provisioning Success acknowledged.");
            break;
        case WIFI_PROV_END:
            ESP_LOGI(TAG, "Provisioning end. Cleaning up BLE.");
            wifi_prov_mgr_deinit();
            break;
        default:
            break;
        }
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
    ESP_LOGW(TAG, "Resetting NVS and Restarting...");
    wifi_prov_mgr_reset_provisioning();
    vTaskDelay(pdMS_TO_TICKS(1000));
    esp_restart();
}

void Provision::button_task(void* pvParameters)
{
    auto* self = static_cast<Provision*>(pvParameters);
    auto consecutive_press_count = std::int32_t{0};

    gpio_config_t io_conf = {};
    io_conf.pin_bit_mask = (1ULL << BOO_BUTTON_GPIO);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    gpio_config(&io_conf);

    while (true)
    {
        if (gpio_get_level(BOO_BUTTON_GPIO) == 0)
        {
            if (++consecutive_press_count >= 4)
            {
                self->forceReProvision();
            }
        }
        else
        {
            consecutive_press_count = 0;
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

} // namespace muc::provision
