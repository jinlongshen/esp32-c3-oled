#include <cstdint>
#include <cstdio>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "I2CBus.h"
#include "I2CDevice.h"
#include "max30102.h"
#include "max30102_regs.h"

namespace
{
const char* TAG = "MAX30102Task";
}

namespace muc
{
namespace max30102
{
void max30102_task(void* pvParameters)
{
    auto* sensor = static_cast<muc::max30102::MAX30102*>(pvParameters);

    // Initialize sensor (you already implemented this)
    if (sensor->initialize() != ESP_OK)
    {
        ESP_LOGE(TAG, "MAX30102 init failed");
        vTaskDelete(nullptr);
    }

    ESP_LOGI(TAG, "MAX30102 initialized, starting sampling...");

    while (true)
    {
        std::uint32_t red = 0;
        std::uint32_t ir = 0;

        esp_err_t err = sensor->readFifoSample(red, ir);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "FIFO read failed: %s", esp_err_to_name(err));
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }

        ESP_LOGI(TAG, "RED=%u  IR=%u", red, ir);

        // ---------------------------------------------------------
        // Add FIFO pointer debug *right here*
        // ---------------------------------------------------------
        uint8_t wptr = 0, rptr = 0;
        sensor->readReg(muc::max30102::Register::FifoWritePtr, wptr);
        sensor->readReg(muc::max30102::Register::FifoReadPtr, rptr);
        ESP_LOGI(TAG, "W=%u  R=%u", wptr, rptr);
        // ---------------------------------------------------------

        vTaskDelay(pdMS_TO_TICKS(10)); // 100 Hz
    }
}

} // namespace max30102
} // namespace muc
