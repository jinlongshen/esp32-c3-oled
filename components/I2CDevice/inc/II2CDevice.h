#ifndef COMPONENT_I2CDEVICE_IIDEVICE_H
#define COMPONENT_I2CDEVICE_IIDEVICE_H

#include <cstdint>
#include <span>

#include "esp_err.h"

namespace muc
{

class II2CDevice
{
  public:
    II2CDevice(const II2CDevice&) = delete;
    II2CDevice& operator=(const II2CDevice&) = delete;
    virtual ~II2CDevice() noexcept = default;

    virtual esp_err_t write(std::span<const std::uint8_t> data) noexcept = 0;
    virtual esp_err_t read(std::span<std::uint8_t> data) noexcept = 0;

  protected:
    II2CDevice() = default;
};

} // namespace muc

#endif // COMPONENT_I2CDEVICE_IIDEVICE_H
