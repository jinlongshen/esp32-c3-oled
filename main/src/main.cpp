#include <array>
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
#include "display_geometry.h"
#include "lvgl_driver.h"
#include "provision.h"
#include "ssd1306.h"
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
    xTaskCreate(muc::ui::UiConsumerTask::ui_init_task, "ui_init", 4 * 1024, &ui_api, 5, nullptr);

    // ---------------------------------------------------------------------
    // 5. Optional font test tasks (kept exactly as requested)
    // ---------------------------------------------------------------------
    // xTaskCreate(muc::fonts::font_test_task, "font_test_task", 2048, &oled, 5, nullptr);
    // xTaskCreate(muc::fonts::font_rotate_task, "font_rotate_task", 2048, &oled, 5, nullptr);

    vTaskDelay(pdMS_TO_TICKS(100));

    // 6. Initialize Provisioning with UI Callbacks
    static muc::provision::Provision provisioning;

    // Pointer to static api to resolve capture warnings
    auto* api_ptr = &ui_api;
    auto* oled_ptr = &oled;

    provisioning.begin(
        [api_ptr, oled_ptr](std::string_view qr_payload)
        {
            oled_ptr->set_scan_mode(true); // Dim & Speed up for camera
            api_ptr->show_provision_qr(qr_payload);
        },
        [api_ptr, oled_ptr](std::string_view ip_address)
        {
            oled_ptr->set_scan_mode(false);  // Restore pretty colors/speed
            api_ptr->set_status(ip_address); // This must also trigger QR deletion
        });

    // 7. MAIN LOOP: Update Counter
    auto i = std::int32_t{0};
    while (true)
    {
        auto buf = std::array<char, 16>{};
        // Use PRIi32 to portably print std::int32_t
        std::snprintf(buf.data(), buf.size(), "%" PRIi32, i++);
        ui_api.set_text(std::string_view{buf.data()});

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}