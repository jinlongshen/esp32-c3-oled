#include <array>
#include <atomic>
#include <cinttypes>
#include <cstdint>
#include <cstdio>
#include <string_view>

#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "Hooks.h"
#include "I2CBus.h"
#include "I2CDevice.h"
#include "ProvisionHooks.h"
#include "display_geometry.h"
#include "lvgl_driver.h"
#include "provision.h"
#include "ssd1306.h"
#include "stockservice.h"
#include "ui_api.h"
#include "ui_consumer_task.h"
#include "ui_queue.h"

namespace
{
constexpr const char* TAG = "MAIN";
}

extern "C" void app_main()
{
    static muc::I2CBus bus(muc::I2C1_PORT, muc::I2C1_SDA_PIN, muc::I2C1_SCL_PIN);
    static muc::I2CDevice oled_slave(bus, muc::ssd1306::OLED_ADDR, muc::I2C1_FREQ);
    static muc::ssd1306::Oled oled(oled_slave, muc::ssd1306::kDefaultGeometry);
    oled.set_scan_mode(true);
    muc::lvgl_driver::lvgl_driver_init(oled);

    // 1. Initialize the Message Queue and API
    static muc::ui::UiQueue ui_queue{20};
    static muc::ui::UiApi ui_api{ui_queue};

    // 2. Configure LVGL Task with your original timing values
    static constexpr muc::ui::LvglTaskConfig lvgl_task_cfg = {
        .tick_period_ms = 20, .handler_period_ms = 40, .user_data = &ui_queue};

    // 3. Start LVGL Tasks with requested stack sizes
    xTaskCreate(muc::ui::UiConsumerTask::lvgl_handler_task,
                "lvgl_handler",
                8 * 1024,
                const_cast<muc::ui::LvglTaskConfig*>(&lvgl_task_cfg),
                5,
                nullptr);

    xTaskCreate(muc::ui::UiConsumerTask::lvgl_tick_task,
                "lvgl_tick",
                2048,
                const_cast<muc::ui::LvglTaskConfig*>(&lvgl_task_cfg),
                5,
                nullptr);

    // 4. Create the labels (Top: Counter, Bottom: Status)
    xTaskCreate(muc::ui::UiConsumerTask::ui_init_task, "ui_init", 4 * 1024, nullptr, 5, nullptr);

    // ---------------------------------------------------------------------
    // 5. Optional font test tasks (kept exactly as requested)
    // ---------------------------------------------------------------------
    // xTaskCreate(muc::fonts::font_test_task, "font_test_task", 2048, &oled, 5, nullptr);
    // xTaskCreate(muc::fonts::font_rotate_task, "font_rotate_task", 2048, &oled, 5, nullptr);

    vTaskDelay(pdMS_TO_TICKS(100));

    // 6. Initialize Provisioning with UI Callbacks
    static muc::provision::Provision provisioning;

    provisioning.begin([&](std::string_view qr_payload)
                       { muc::provisionhooks::on_qr(ui_api, oled, qr_payload); },
                       [&](std::string_view ip_address)
                       { muc::provisionhooks::on_success(ui_api, oled, ip_address); });

    // 7. MAIN LOOP: Update Counter + Fetch Stock (when allowed)
    auto i = std::int32_t{0};
    muc::stock::StockQuote q{};

    while (true)
    {
        // Fetch stock price only when allowed
        if (muc::provisionhooks::g_can_fetch_stock.load())
        {
            if (muc::stock::fetch_nvda_price(q))
            {
                ESP_LOGI("MAIN", "NVDA: %.2f USD", q.price);
            }
            else
            {
                ESP_LOGE("MAIN", "Failed to fetch NVDA price");
            }
        }

        // Build two-line display buffer
        auto buf = std::array<char, 32>{};

        std::snprintf(buf.data(), buf.size(), "%" PRIi32 "\nNividia %.2f", i++, q.price);

        ui_api.set_text(std::string_view{buf.data()});

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
