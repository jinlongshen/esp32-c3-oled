#include "max30102.h"
#include <span>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "max30102_regs.h"

namespace muc
{
namespace max30102
{
MAX30102::MAX30102(I2CDevice& dev) noexcept
: m_dev(dev)
{
}

esp_err_t MAX30102::initialize() noexcept
{
    // --- Reset the device ---
    esp_err_t err = writeReg(Register::ModeConfig, ModeConfigBits::RESET);
    if (err != ESP_OK)
    {
        return err;
    }

    vTaskDelay(pdMS_TO_TICKS(10));

    // --- FIFO config ---
    // Sample averaging = 4 (0b010), FIFO rollover enabled, almost full = 0x0F
    // 0x4F = 0b0100'1111
    err = writeReg(Register::FifoConfig, 0x4F);
    if (err != ESP_OK)
    {
        return err;
    }

    // --- LED mode: RED + IR ---
    err = setLedMode(LedMode::Spo2RedIr);
    if (err != ESP_OK)
    {
        return err;
    }

    // --- SPO2 config ---
    // ADC range = 4096 nA (0b01 << 5)
    // Sample rate = 100 Hz (0b011 << 2)
    // Pulse width = 411 us (0b11)
    // Combined = 0x27
    err = writeReg(Register::Spo2Config, 0x27);
    if (err != ESP_OK)
    {
        return err;
    }

    // --- LED pulse amplitudes ---
    // 0x24 = moderate LED current
    err = writeReg(Register::Led1PulseAmp, 0x24); // IR LED
    if (err != ESP_OK)
    {
        return err;
    }

    err = writeReg(Register::Led2PulseAmp, 0x24); // RED LED
    if (err != ESP_OK)
    {
        return err;
    }

    return ESP_OK;
}

esp_err_t MAX30102::writeReg(Register reg, std::uint8_t value) noexcept
{
    std::uint8_t buf[2] = {max30102::reg(reg), value};
    return m_dev.write(buf);
}

esp_err_t MAX30102::readReg(Register reg, std::uint8_t& value) noexcept
{
    std::uint8_t r = max30102::reg(reg);
    esp_err_t err = m_dev.write(std::span<const std::uint8_t>(&r, 1));
    if (err != ESP_OK)
    {
        return err;
    }

    return m_dev.read(std::span<std::uint8_t>(&value, 1));
}

esp_err_t MAX30102::reset() noexcept
{
    return writeReg(Register::ModeConfig, ModeConfigBits::RESET);
}

esp_err_t MAX30102::readPartId(std::uint8_t& id) noexcept
{
    return readReg(Register::PartId, id);
}

esp_err_t MAX30102::setLedMode(LedMode mode) noexcept
{
    std::uint8_t regval = 0;
    readReg(Register::ModeConfig, regval);

    regval &= ~0x07; // clean mode bits
    regval |= static_cast<std::uint8_t>(mode);

    return writeReg(Register::ModeConfig, regval);
}

esp_err_t MAX30102::setSpo2Config(std::uint8_t config) noexcept
{
    return writeReg(Register::Spo2Config, config);
}

esp_err_t MAX30102::setFifoConfig(std::uint8_t config) noexcept
{
    return writeReg(Register::FifoConfig, config);
}

esp_err_t MAX30102::readTemperature(float& celsius) noexcept
{
    // Start conversion
    esp_err_t err = writeReg(Register::TempConfig, TempConfigBits::TEMP_EN);
    if (err != ESP_OK)
    {
        return err;
    }

    // Wait for DIE_TEMP_RDY
    std::uint8_t status = 0;
    for (int i = 0; i < 100; ++i)
    {
        readReg(Register::IntStatus2, status);
        if (status & Int2Bits::DIE_TEMP_RDY)
        {
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }

    std::uint8_t ti = 0, tf = 0;
    readReg(Register::TempInt, ti);
    readReg(Register::TempFrac, tf);

    celsius = float(ti) + float(tf) * 0.0625f;
    return ESP_OK;
}

esp_err_t MAX30102::readFifoSample(std::uint32_t& red, std::uint32_t& ir) noexcept
{
    std::uint8_t addr = max30102::reg(Register::FifoData);
    esp_err_t err = m_dev.write(std::span<const std::uint8_t>(&addr, 1));
    if (err != ESP_OK)
    {
        return err;
    }
    std::uint8_t raw[6] = {0};
    err = m_dev.read(std::span<std::uint8_t>(raw, 6));
    if (err != ESP_OK)
    {
        return err;
    }

    // MAX30102 stores samples MSB-first, 18-bit values
    ir = ((raw[0] << 16) | (raw[1] << 8) | raw[2]) & 0x3FFFF;
    red = ((raw[3] << 16) | (raw[4] << 8) | raw[5]) & 0x3FFFF;

    return ESP_OK;
}
} // namespace max30102
} // namespace muc
