#ifndef COMPONENTS_MAX30102_H
#define COMPONENTS_MAX30102_H

#include <cstdint>
#include "esp_err.h"
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
    esp_err_t reset() noexcept;

    esp_err_t readPartId(std::uint8_t& id) noexcept;

    // Configuration APIs
    esp_err_t setLedMode(LedMode mode) noexcept;
    esp_err_t setAdcRange(AdcRange range) noexcept;
    esp_err_t setSampleRate(SampleRate rate) noexcept;
    esp_err_t setPulseWidth(PulseWidth width) noexcept;
    esp_err_t setSampleAveraging(std::uint8_t avg) noexcept;

    esp_err_t setLedPulseAmplitudeRed(std::uint8_t amp) noexcept;
    esp_err_t setLedPulseAmplitudeIR(std::uint8_t amp) noexcept;

    esp_err_t setSpo2Config(std::uint8_t config) noexcept;
    esp_err_t setFifoConfig(std::uint8_t config) noexcept;

    // Sensor data
    esp_err_t readTemperature(float& celsius) noexcept;
    esp_err_t readFifoSample(std::uint32_t& red, std::uint32_t& ir) noexcept;

    // Low-level register access
    esp_err_t writeReg(Register reg, std::uint8_t value) noexcept;
    esp_err_t readReg(Register reg, std::uint8_t& value) noexcept;

  private:
    I2CDevice& m_dev;
};

} // namespace max30102
} // namespace muc

#endif // COMPONENTS_MAX30102_H
