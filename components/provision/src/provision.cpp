#include "provision.h"

#include <esp_event.h>
#include <esp_log.h>
#include <esp_wifi.h>
#include <nvs_flash.h>
#include <driver/gpio.h>
#include <wifi_provisioning/manager.h>
#include <wifi_provisioning/scheme_ble.h>

namespace muc::provision
{

static const char* TAG = "PROVISION";
static constexpr gpio_num_t BOO_BUTTON_GPIO = GPIO_NUM_9;

Provision::Provision()
{
    _wifi_event_group = xEventGroupCreate();
}

esp_err_t Provision::begin()
{
    // 1. Initialize NVS (Requirement for Wi-Fi/BT)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // 2. Network Interface & Event Loop
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    // 3. Register Event Handlers
    ESP_ERROR_CHECK(esp_event_handler_register(
        WIFI_PROV_EVENT, ESP_EVENT_ANY_ID, &Provision::event_handler, this));
    ESP_ERROR_CHECK(
        esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &Provision::event_handler, this));
    ESP_ERROR_CHECK(
        esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &Provision::event_handler, this));

    // 4. Initialize Wi-Fi
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // 5. Provisioning Manager Config (C++ Zero-init style)
    wifi_prov_mgr_config_t config = {};
    config.scheme = wifi_prov_scheme_ble;
    config.scheme_event_handler = WIFI_PROV_SCHEME_BLE_EVENT_HANDLER_FREE_BTDM;

    ESP_ERROR_CHECK(wifi_prov_mgr_init(config));

    // 6. Check if already provisioned
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

    // 7. Start the 500ms polling button task
    xTaskCreate(Provision::button_task, "prov_btn_task", 4096, this, 5, NULL);

    return ESP_OK;
}

void Provision::start_provisioning()
{
    ESP_LOGI(TAG, "Starting BLE Provisioning...");

    char service_name[12];
    get_device_service_name(service_name, sizeof(service_name));

    // Security level 1 with a simple PoP (Proof of Possession)
    wifi_prov_security_t security = WIFI_PROV_SECURITY_1;
    const char* pop = "abcd1234";

    ESP_ERROR_CHECK(wifi_prov_mgr_start_provisioning(security, (void*)pop, service_name, NULL));
    ESP_LOGI(TAG, "Provisioning started. Name: %s, POP: %s", service_name, pop);
}

void Provision::wait_for_connection()
{
    xEventGroupWaitBits(_wifi_event_group, CONNECTED_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
}

void Provision::event_handler(void* arg, esp_event_base_t base, int32_t id, void* data)
{
    Provision* self = static_cast<Provision*>(arg);

    if (base == WIFI_PROV_EVENT && id == WIFI_PROV_CRED_SUCCESS)
    {
        ESP_LOGI(TAG, "Provisioning successful");
    }
    else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    }
    else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP)
    {
        ESP_LOGI(TAG, "Got IP Address");
        xEventGroupSetBits(self->_wifi_event_group, CONNECTED_BIT);
    }
}

void Provision::get_device_service_name(char* service_name, size_t max)
{
    uint8_t eth_mac[6];
    esp_wifi_get_mac(WIFI_IF_STA, eth_mac);
    snprintf(service_name, max, "PROV_%02X%02X%02X", eth_mac[3], eth_mac[4], eth_mac[5]);
}

void Provision::forceReProvision()
{
    ESP_LOGI(TAG, "Clearing Wi-Fi credentials and rebooting to Provision Mode...");
    wifi_prov_mgr_reset_provisioning();
    vTaskDelay(pdMS_TO_TICKS(100));
    esp_restart();
}

void Provision::button_task(void* pvParameters)
{
    Provision* self = static_cast<Provision*>(pvParameters);
    int consecutive_press_count = 0;
    const int REQUIRED_COUNT = 4; // 4 * 500ms = 2 seconds
    const int POLL_PERIOD_MS = 500;

    // Configure GPIO 9 (C++ Zero-init style to satisfy compiler)
    gpio_config_t io_conf = {};
    io_conf.pin_bit_mask = (1ULL << BOO_BUTTON_GPIO);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.intr_type = GPIO_INTR_DISABLE;

    gpio_config(&io_conf);

    ESP_LOGI(TAG, "Button monitor started (Polling: %dms)", POLL_PERIOD_MS);

    while (true)
    {
        // Read BOO button (Active Low)
        if (gpio_get_level(BOO_BUTTON_GPIO) == 0)
        {
            consecutive_press_count++;

            if (consecutive_press_count >= REQUIRED_COUNT)
            {
                ESP_LOGW(TAG, "2-second threshold reached! Resetting...");
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