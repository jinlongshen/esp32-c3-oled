#ifndef COMPONENTS_I2CDEVICE_I2CBUS_H
#define COMPONENTS_I2CDEVICE_I2CBUS_H

#include <cstdint>
#include <mutex>

#include "II2CBus.h"
#include "driver/i2c_master.h"

namespace muc
{

static constexpr i2c_port_t I2C1_PORT = I2C_NUM_0;
static constexpr gpio_num_t I2C1_SDA_PIN = GPIO_NUM_5;
static constexpr gpio_num_t I2C1_SCL_PIN = GPIO_NUM_6;

class I2CBus : public II2CBus
{
  public:
    I2CBus(i2c_port_t port, gpio_num_t sda, gpio_num_t scl) noexcept;
    virtual ~I2CBus() noexcept override final;

    virtual i2c_master_bus_handle_t handle() const noexcept override final;

    std::mutex& mutex() noexcept override final;

  private:
    i2c_master_bus_handle_t m_handle;
    std::mutex m_mutex;
};

} // namespace muc

#endif // COMPONENTS_I2CDEVICE_I2CBUS_H
