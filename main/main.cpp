#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ssd1306.h"

extern "C" void app_main(void);

void app_main()
{

    muc::ssd1306::Oled oled;
    oled.drawVisibleBar();
    oled.drawCharA(32, 12);
    while (true)
    {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}