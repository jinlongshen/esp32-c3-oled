#include "max30102.h"
#include "max30102_regs.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <array>
#include <span>

namespace muc
{
namespace max30102
{

static constexpr char TAG[] = "MAX30102";

MAX30102::MAX30102(I2CDevice& dev) noexcept
: m_dev(dev)
{
}

// -----------------------------------------------------------------------------
// Low-level register access
// -----------------------------------------------------------------------------

esp_err_t MAX30102::writeReg(Register reg, std::uint8_t value) noexcept
{
    std::array<std::uint8_t, 2> buf{static_cast<std::uint8_t>(reg), value};

    return m_dev.write(std::span<const std::uint8_t>(buf));
}

esp_err_t MAX30102::readReg(Register reg, std::uint8_t& value) noexcept
{
    // Write register address
    std::array<std::uint8_t, 1> addr{static_cast<std::uint8_t>(reg)};

    esp_err_t err = m_dev.write(std::span<const std::uint8_t>(addr));
    if (err != ESP_OK)
    {
        return err;
    }

    // Read one byte
    std::array<std::uint8_t, 1> data{};
    err = m_dev.read(std::span<std::uint8_t>(data));
    if (err != ESP_OK)
    {
        return err;
    }

    value = data[0];
    return ESP_OK;
}

// -----------------------------------------------------------------------------
// Reset & Part ID
// -----------------------------------------------------------------------------

esp_err_t MAX30102::reset() noexcept
{
    esp_err_t err = writeReg(Register::ModeConfig, ModeConfigBits::RESET);
    if (err != ESP_OK)
    {
        return err;
    }

    vTaskDelay(pdMS_TO_TICKS(10));
    return ESP_OK;
}

esp_err_t MAX30102::readPartId(std::uint8_t& id) noexcept
{
    return readReg(Register::PartId, id);
}

// -----------------------------------------------------------------------------
// Configuration APIs
// -----------------------------------------------------------------------------

esp_err_t MAX30102::setLedMode(LedMode mode) noexcept
{
    return writeReg(Register::ModeConfig, static_cast<std::uint8_t>(mode));
}

esp_err_t MAX30102::setAdcRange(AdcRange range) noexcept
{
    std::uint8_t regValue{};
    esp_err_t err = readReg(Register::Spo2Config, regValue);
    if (err != ESP_OK)
    {
        return err;
    }

    regValue &= ~Spo2ConfigBits::ADC_RGE_MASK;
    regValue |= (static_cast<std::uint8_t>(range) << Spo2ConfigBits::ADC_RGE_SHIFT);

    return writeReg(Register::Spo2Config, regValue);
}

esp_err_t MAX30102::setSampleRate(SampleRate rate) noexcept
{
    std::uint8_t regValue{};
    esp_err_t err = readReg(Register::Spo2Config, regValue);
    if (err != ESP_OK)
    {
        return err;
    }

    regValue &= ~Spo2ConfigBits::SR_MASK;
    regValue |= (static_cast<std::uint8_t>(rate) << Spo2ConfigBits::SR_SHIFT);

    return writeReg(Register::Spo2Config, regValue);
}

esp_err_t MAX30102::setPulseWidth(PulseWidth width) noexcept
{
    std::uint8_t regValue{};
    esp_err_t err = readReg(Register::Spo2Config, regValue);
    if (err != ESP_OK)
    {
        return err;
    }

    regValue &= ~Spo2ConfigBits::LED_PW_MASK;
    regValue |= static_cast<std::uint8_t>(width);

    return writeReg(Register::Spo2Config, regValue);
}

esp_err_t MAX30102::setSampleAveraging(std::uint8_t avg) noexcept
{
    std::uint8_t regValue{};
    esp_err_t err = readReg(Register::FifoConfig, regValue);
    if (err != ESP_OK)
    {
        return err;
    }

    regValue &= ~FifoConfigBits::SMP_AVE_MASK;
    regValue |= (avg << FifoConfigBits::SMP_AVE_SHIFT);

    return writeReg(Register::FifoConfig, regValue);
}

esp_err_t MAX30102::setLedPulseAmplitudeRed(std::uint8_t amp) noexcept
{
    return writeReg(Register::Led2PulseAmp, amp);
}

esp_err_t MAX30102::setLedPulseAmplitudeIR(std::uint8_t amp) noexcept
{
    return writeReg(Register::Led1PulseAmp, amp);
}

esp_err_t MAX30102::setSpo2Config(std::uint8_t config) noexcept
{
    return writeReg(Register::Spo2Config, config);
}

esp_err_t MAX30102::setFifoConfig(std::uint8_t config) noexcept
{
    return writeReg(Register::FifoConfig, config);
}

// -----------------------------------------------------------------------------
// Temperature
// -----------------------------------------------------------------------------

esp_err_t MAX30102::readTemperature(float& celsius) noexcept
{
    std::uint8_t intPart{};
    std::uint8_t fracPart{};

    esp_err_t err = readReg(Register::TempInt, intPart);
    if (err != ESP_OK)
    {
        return err;
    }

    err = readReg(Register::TempFrac, fracPart);
    if (err != ESP_OK)
    {
        return err;
    }

    celsius = static_cast<float>(intPart) + (static_cast<float>(fracPart) * 0.0625f);
    return ESP_OK;
}

// -----------------------------------------------------------------------------
// FIFO sample read
// -----------------------------------------------------------------------------

esp_err_t MAX30102::readFifoSample(std::uint32_t& red, std::uint32_t& ir) noexcept
{
    // Write FIFO address
    std::array<std::uint8_t, 1> addr{static_cast<std::uint8_t>(Register::FifoData)};

    esp_err_t err = m_dev.write(std::span<const std::uint8_t>(addr));
    if (err != ESP_OK)
    {
        return err;
    }

    // Read 6 bytes
    std::array<std::uint8_t, 6> buf{};
    err = m_dev.read(std::span<std::uint8_t>(buf));
    if (err != ESP_OK)
    {
        return err;
    }

    red = ((static_cast<std::uint32_t>(buf[0]) & 0x03u) << 16) |
          (static_cast<std::uint32_t>(buf[1]) << 8) | (static_cast<std::uint32_t>(buf[2]));

    ir = ((static_cast<std::uint32_t>(buf[3]) & 0x03u) << 16) |
         (static_cast<std::uint32_t>(buf[4]) << 8) | (static_cast<std::uint32_t>(buf[5]));

    return ESP_OK;
}

// -----------------------------------------------------------------------------
// Initialization
// -----------------------------------------------------------------------------

esp_err_t MAX30102::initialize() noexcept
{
    ESP_LOGI(TAG, "Initializing MAX30102");

    esp_err_t err = reset();
    if (err != ESP_OK)
    {
        return err;
    }

    // FIFO: averaging=4, rollover, almost full=0x0F
    err = setFifoConfig(FifoConfigBits::SMP_AVE_4 | FifoConfigBits::ROLLOVER_EN | 0x0F);
    if (err != ESP_OK)
    {
        return err;
    }

    // LED mode: RED + IR
    err = setLedMode(LedMode::Spo2RedIr);
    if (err != ESP_OK)
    {
        return err;
    }

    // SPO2 config: ADC=16384nA, SR=50Hz, PW=411us
    err = setAdcRange(AdcRange::Range16384);
    if (err != ESP_OK)
    {
        return err;
    }

    err = setSampleRate(SampleRate::Rate50);
    if (err != ESP_OK)
    {
        return err;
    }

    err = setPulseWidth(PulseWidth::Us411);
    if (err != ESP_OK)
    {
        return err;
    }

    // LED currents: strong for HR detection
    err = setLedPulseAmplitudeIR(0xFF);
    if (err != ESP_OK)
    {
        return err;
    }

    err = setLedPulseAmplitudeRed(0xFF);
    if (err != ESP_OK)
    {
        return err;
    }

    return ESP_OK;
}

} // namespace max30102
} // namespace muc
