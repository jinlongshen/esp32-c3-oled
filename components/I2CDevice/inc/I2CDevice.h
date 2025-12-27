#ifndef COMPONENTS_I2CDEVICE_I2CDEVICE_H
#define COMPONENTS_I2CDEVICE_I2CDEVICE_H

#include "II2CBus.h"
#include "II2CDevice.h"
#include "driver/i2c_master.h"

namespace muc
{

static constexpr uint32_t I2C1_FREQ = 400000;
static constexpr uint8_t OLED_ADDR = 0x3C;

class I2CDevice : public II2CDevice
{
  public:
    I2CDevice(II2CBus& bus,
              std::uint8_t address,
              std::uint32_t freq_hz) noexcept;
    virtual ~I2CDevice() noexcept override final;

    virtual esp_err_t write(
        std::span<const std::uint8_t> data) noexcept override final;

  private:
    II2CBus& m_bus;
    i2c_master_dev_handle_t m_dev;
};

} // namespace muc

#endif