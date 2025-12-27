#ifndef COMPONENTS_I2CDEVICE_II2CBUS_H
#define COMPONENTS_I2CDEVICE_II2CBUS_H

#include "driver/i2c_master.h"

namespace muc
{

class II2CBus
{
  public:
    II2CBus(const II2CBus&) = delete;
    II2CBus& operator=(const II2CBus&) = delete;
    virtual ~II2CBus() noexcept = default;
    [[nodiscard]] virtual i2c_master_bus_handle_t handle() const noexcept = 0;

  protected:
    II2CBus() noexcept = default;
};

} // namespace muc

#endif // COMPONENTS_I2CDEVICE_II2CBUS_H