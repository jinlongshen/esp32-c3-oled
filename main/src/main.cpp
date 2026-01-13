#include <cstdio>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <esp_log.h>

#include "Hooks.h"
#include "I2CBus.h"
#include "I2CDevice.h"
#include "display_geometry.h"
#include "lvgl_driver.h"
#include "provision.h"
#include "ssd1306.h"
#include "ui_api.h"
#include "ui_consumer_task.h"
#include "ui_queue.h"

namespace muc::fonts
{
void font_task(void* pvParameters);
void font_test_task(void* pvParameters);
void font_rotate_task(void* pvParameters);
} // namespace muc::fonts

static inline const char* itostring_digit(int v)
{
    static char buf[2];
    buf[0] = '0' + (v % 10);
    buf[1] = '\0';
    return buf;
}

extern "C" void app_main(void)
{
    esp_log_level_set("UI_API", ESP_LOG_WARN);
    esp_log_level_set("UI_QUEUE", ESP_LOG_WARN);
    esp_log_level_set("PROVISION", ESP_LOG_INFO);

    // ---------------------------------------------------------------------
    // 0. Provisioning & Wi-Fi Initialization
    // ---------------------------------------------------------------------
    // Instantiate the Provision manager. Static ensures it lives for the app duration.
    static muc::provision::Provision provision_mgr;

    // Initialize NVS, Network, and spawn the button monitoring task (GPIO 9).
    provision_mgr.begin();

    // ---------------------------------------------------------------------
    // 1. Hardware initialization
    // ---------------------------------------------------------------------
    static muc::I2CBus bus(muc::I2C1_PORT, muc::I2C1_SDA_PIN, muc::I2C1_SCL_PIN);
    static muc::I2CDevice oled_slave(bus, muc::ssd1306::OLED_ADDR, muc::I2C1_FREQ);
    static muc::ssd1306::Oled oled(oled_slave, muc::ssd1306::kDefaultGeometry);

    // ---------------------------------------------------------------------
    // 2. LVGL initialization (driver only)
    // ---------------------------------------------------------------------
    muc::lvgl_driver::lvgl_driver_init(oled);

    // ---------------------------------------------------------------------
    // 3. UI queue + API
    // ---------------------------------------------------------------------
    static muc::ui::UiQueue ui_queue(10);
    static muc::ui::UiApi ui_api(ui_queue);

    static muc::ui::LvglTaskConfig lvgl_task_cfg{
        .tick_period_ms = 20,
        .handler_period_ms = 40,
        .user_data = &ui_queue
    };

    // ---------------------------------------------------------------------
    // 4. Start LVGL tasks
    // ---------------------------------------------------------------------
    xTaskCreate(
        muc::ui::UiConsumerTask::lvgl_tick_task,
        "lvgl_tick_task",
        2048,
        &lvgl_task_cfg,
        1,
        nullptr);

    xTaskCreate(
        muc::ui::UiConsumerTask::lvgl_handler_task,
        "lvgl_handler_task",
        8192,
        &lvgl_task_cfg,
        2,
        nullptr);

    // ---------------------------------------------------------------------
    // 5. Start UI initialization task
    // ---------------------------------------------------------------------
    xTaskCreate(
        muc::ui::UiConsumerTask::ui_init_task,
        "ui_init_task",
        4096,
        &ui_api,
        3,
        nullptr);

    // ---------------------------------------------------------------------
    // 6. Optional font test tasks (kept exactly as requested)
    // ---------------------------------------------------------------------
    // xTaskCreate(muc::fonts::font_test_task, "font_test_task", 20480, &oled, 5, nullptr);
    // xTaskCreate(muc::fonts::font_rotate_task, "font_rotate_task", 20480, &oled, 5, nullptr);

    // ---------------------------------------------------------------------
    // 7. Start stack monitor task
    // ---------------------------------------------------------------------
    xTaskCreate(
        stack_monitor_task,
        "stack_monitor",
        4096,
        nullptr,
        1,
        nullptr);

    // ---------------------------------------------------------------------
    // 8. Idle loop
    // ---------------------------------------------------------------------
    int i = 0;
    while (true)
    {
        vTaskDelay(pdMS_TO_TICKS(2000));
        ESP_LOGI("DBG", "alive");

        // Update main label
        ui_api.set_text(itostring_digit(i % 10));

        // Update status label
        ui_api.set_status("OK");

        // Update CPU percentage (fake load)
        ui_api.set_cpu_usage((i * 13) % 100);

        i++;
    }
}