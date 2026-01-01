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

    while (true)
    {
        // ---------------------------------------------------------------------
        // 1. Dummy write to a writable register (FIFO write pointer)
        // ---------------------------------------------------------------------
        constexpr std::uint8_t test_value = 0x12;

        esp_err_t err = sensor->writeReg(muc::max30102::Register::FifoWritePtr, test_value);

        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "I2C write failed: %s", esp_err_to_name(err));
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        // ---------------------------------------------------------------------
        // 2. Read back the same register
        // ---------------------------------------------------------------------
        std::uint8_t read_back = 0;
        err = sensor->readReg(muc::max30102::Register::FifoWritePtr, read_back);

        if (err == ESP_OK)
        {
            ESP_LOGI(TAG, "FIFO_WRITE_PTR wrote 0x%02X, read back 0x%02X", test_value, read_back);
        }
        else
        {
            ESP_LOGE(TAG, "I2C read failed: %s", esp_err_to_name(err));
        }

        // ---------------------------------------------------------------------
        // 3. Read PART ID (should be 0x15)
        // ---------------------------------------------------------------------
        std::uint8_t part_id = 0;
        err = sensor->readReg(muc::max30102::Register::PartId, part_id);

        if (err == ESP_OK)
        {
            ESP_LOGI(TAG, "MAX30102 PART ID = 0x%02X", part_id);
        }
        else
        {
            ESP_LOGE(TAG, "Failed to read PART ID: %s", esp_err_to_name(err));
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
} // namespace max30102
} // namespace muc
