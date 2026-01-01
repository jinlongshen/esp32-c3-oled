#include "max30102_task.h"
#include "max30102.h"
#include "max30102_regs.h"
#include "spo2_hr.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

namespace
{
const char* TAG = "MAX30102Task";
}

// Processor must NOT be on task stack
static muc::max30102::SpO2HrProcessor s_spo2HrProcessor;

namespace muc
{
namespace max30102
{

void max30102_task(void* pvParameters)
{
    auto* sensor = static_cast<MAX30102*>(pvParameters);

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

        ESP_LOGI(TAG, "RED=%u IR=%u", red, ir);

        s_spo2HrProcessor.addSample(red, ir);

        // float spo2 = 0.0f;
        // if (s_spo2HrProcessor.computeSpO2(spo2))
        // {
        //     ESP_LOGI(TAG, "SpO2 = %.2f %%", spo2);
        // }

        float bpm = 0.0f;
        if (s_spo2HrProcessor.computeHeartRate(bpm))
        {
            ESP_LOGI(TAG, "Heart Rate = %.1f BPM", bpm);
        }

        vTaskDelay(pdMS_TO_TICKS(20)); // 50 Hz sampling
    }
}

} // namespace max30102
} // namespace muc
