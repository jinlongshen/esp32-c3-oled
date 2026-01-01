#include "I2CBus.h"

namespace muc
{

I2CBus::I2CBus(i2c_port_t port, gpio_num_t sda, gpio_num_t scl) noexcept
: m_handle(nullptr)
, m_mutex()
{
    i2c_master_bus_config_t cfg = {};
    cfg.i2c_port = port;
    cfg.sda_io_num = sda;
    cfg.scl_io_num = scl;
    cfg.clk_source = I2C_CLK_SRC_DEFAULT;
    cfg.glitch_ignore_cnt = 7;
    cfg.flags.enable_internal_pullup = true;

    // Fail fast if the bus cannot be created
    const esp_err_t err = i2c_new_master_bus(&cfg, &m_handle);
    ESP_ERROR_CHECK(err);
}

I2CBus::~I2CBus() noexcept
{
    if (m_handle)
    {
        i2c_del_master_bus(m_handle);
    }
}

i2c_master_bus_handle_t I2CBus::handle() const noexcept
{
    return m_handle;
}

std::mutex& I2CBus::mutex() noexcept
{
    return m_mutex;
}

} // namespace muc
