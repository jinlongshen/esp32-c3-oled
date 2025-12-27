#include "I2CDevice.h"

#include "driver/i2c_master.h"

namespace muc
{

I2CDevice::I2CDevice(II2CBus& bus,
                     std::uint8_t address,
                     std::uint32_t freq_hz) noexcept
: m_bus(bus)
, m_dev(nullptr)
{
    i2c_device_config_t cfg = {};
    cfg.device_address = address;
    cfg.scl_speed_hz = freq_hz;

    const esp_err_t err =
        i2c_master_bus_add_device(m_bus.handle(), &cfg, &m_dev);
    ESP_ERROR_CHECK(err);
}

I2CDevice::~I2CDevice() noexcept
{
    if (m_dev)
    {
        i2c_master_bus_rm_device(m_dev);
    }
}

esp_err_t I2CDevice::write(std::span<const std::uint8_t> data) noexcept
{
    return i2c_master_transmit(m_dev, data.data(), data.size(), -1);
}

} // namespace muc
