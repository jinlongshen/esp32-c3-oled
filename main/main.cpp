#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "I2CBus.h"
#include "I2CDevice.h"
#include "ssd1306.h"

// Declare the task from the other file
namespace muc::fonts
{
void font_task(void* pvParameters);
}

extern "C" void app_main(void)
{
    // Initialize Hardware
    static muc::I2CBus bus(muc::I2C1_PORT, muc::I2C1_SDA_PIN, muc::I2C1_SCL_PIN);
    static muc::I2CDevice dev(bus, muc::OLED_ADDR, muc::I2C1_FREQ);
    static muc::ssd1306::Oled oled(dev);

    // Start the font animation task with enough stack
    xTaskCreate(muc::fonts::font_task, "font_task", 20480, &oled, 5, nullptr);
}