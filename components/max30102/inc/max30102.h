#pragma once

#include <cstdint>
#include "I2CDevice.h"
#include "max30102_regs.h"

namespace muc
{
namespace max30102
{
class MAX30102
{
  public:
    explicit MAX30102(I2CDevice& dev) noexcept;

    esp_err_t initialize() noexcept;

    // Basic device checks
    esp_err_t reset() noexcept;
    esp_err_t readPartId(std::uint8_t& id) noexcept;

    // Temperature
    esp_err_t readTemperature(float& celsius) noexcept;

    // Register access helpers
    esp_err_t writeReg(max30102::Register reg, std::uint8_t value) noexcept;
    esp_err_t readReg(max30102::Register reg, std::uint8_t& value) noexcept;

    // Configuration
    esp_err_t setLedMode(max30102::LedMode mode) noexcept;
    esp_err_t setSpo2Config(std::uint8_t config) noexcept;
    esp_err_t setFifoConfig(std::uint8_t config) noexcept;

    // FIFO
    esp_err_t readFifoSample(std::uint32_t& red, std::uint32_t& ir) noexcept;

  private:
    I2CDevice& m_dev;
};
} // namespace max30102
} // namespace muc
