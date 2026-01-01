#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "I2CBus.h"
#include "I2CDevice.h"
#include "ssd1306.h"
#include "max30102.h"
#include "max30102_task.h"

// Declare the task from the other file
namespace muc::fonts
{
void font_task(void* pvParameters);
}

extern "C" void app_main(void)
{
    // Initialize Hardware
    static muc::I2CBus bus(muc::I2C1_PORT, muc::I2C1_SDA_PIN, muc::I2C1_SCL_PIN);
    static muc::I2CDevice oled_slave(bus, muc::ssd1306::OLED_ADDR, muc::I2C1_FREQ);
    static muc::I2CDevice max30102_slave(bus, muc::max30102::I2C_ADDRESS, muc::I2C1_FREQ);

    static muc::ssd1306::Oled oled(oled_slave);
    static muc::max30102::MAX30102 max30102(max30102_slave);
    max30102.initialize();

    // Start the font animation task with enough stack
    xTaskCreate(muc::fonts::font_task, "font_task", 20480, &oled, 5, nullptr);

    xTaskCreate(muc::max30102::max30102_task,
                "max30102_task",
                8192, // bytes on ESP32-C3
                &max30102,
                3,
                nullptr);
}