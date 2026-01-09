#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "I2CBus.h"
#include "I2CDevice.h"
#include "ssd1306.h"
#include "lvgl_driver.h"
#include "lvgl_display.h"
#include "lvgl_tasks.h"

namespace muc::fonts
{
void font_task(void* pvParameters);
void font_test_task(void* pvParameters);
void font_rotate_task(void* pvParameters);
}

extern "C" void app_main(void)
{
    // Initialize Hardware
    static muc::I2CBus bus(muc::I2C1_PORT, muc::I2C1_SDA_PIN, muc::I2C1_SCL_PIN);

    static muc::I2CDevice oled_slave(bus, muc::ssd1306::OLED_ADDR, muc::I2C1_FREQ);

    static muc::ssd1306::Oled oled(oled_slave);

    // Initialize LVGL and register display driver
    muc::lvgl_driver::init(oled);
    xTaskCreate(muc::lvgl_driver::lvgl_handler_task, "lvgl_handler_task", 20480, &oled, 5, nullptr);
    xTaskCreate(muc::lvgl_driver::lvgl_tick_task, "lvgl_tick_task", 2048, nullptr, 3, nullptr);

    // xTaskCreate(muc::fonts::font_test_task, "font_test_task", 20480, &oled, 5, nullptr);
    // xTaskCreate(muc::fonts::font_rotate_task, "font_rotate_task", 20480, &oled, 5, nullptr);

    // -------------------------------------------------------------------------
    // Idle loop
    // -------------------------------------------------------------------------
    while (true)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
