#include "I2CBus.h"
#include "I2CDevice.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ssd1306.h"

extern "C" void app_main(void);

void app_main()
{
    muc::I2CBus bus(muc::I2C1_PORT, muc::I2C1_SDA_PIN, muc::I2C1_SCL_PIN);
    muc::I2CDevice device(bus, muc::OLED_ADDR, muc::I2C1_FREQ);
    muc::ssd1306::Oled oled(device);
    oled.drawVisibleBar();
    oled.drawCharA(16, 24);
    while (true)
    {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}