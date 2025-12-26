#include "ssd1306.h"

#include <array>
#include <cstdint>
#include <cstring>

#include "esp_log.h"

namespace
{
static const char* TAG = "OLED_SSD1306_ABROBOT";
}

namespace muc
{
namespace ssd1306
{

Oled::Oled()
: m_bus_handle(nullptr)
, m_dev_handle(nullptr)
{
    initI2C();
    ping();
    initialize();
}

void Oled::drawVisibleBar() noexcept
{
    std::uint8_t page = (OLED_Y_OFFSET / 8);
    std::array<std::uint8_t, 32> bar;
    bar.fill(0xFF);
    setPageColumn(page, OLED_X_OFFSET);
    sendData(bar.data(), bar.size());
    ESP_LOGI(TAG, "Drew white bar at x=%d..%d, page=%d (y≈%d..%d)",
             OLED_X_OFFSET, OLED_X_OFFSET + bar.size() - 1, page, page * 8,
             page * 8 + 7);
}

void Oled::drawCharA(int x, int y)
{
    constexpr std::array<std::uint8_t, 5> A_5x8 = {0x7C, 0x12, 0x11, 0x12,
                                                   0x7C};
    const int page = (OLED_Y_OFFSET + y) / 8;
    const int column = OLED_X_OFFSET + x;
    setPageColumn(page, column);
    sendData(A_5x8.data(), 5);
}

void Oled::initI2C()
{
    ESP_LOGI(TAG, "Initializing I2C master (new ESP-IDF 6.x API)");

    i2c_master_bus_config_t bus_cfg = {};

    bus_cfg.clk_source = I2C_CLK_SRC_DEFAULT;
    bus_cfg.i2c_port = s_OLED_I2C_PORT;
    bus_cfg.sda_io_num = OLED_SDA_PIN;
    bus_cfg.scl_io_num = OLED_SCL_PIN;
    bus_cfg.glitch_ignore_cnt = 7;
    bus_cfg.flags.enable_internal_pullup = true;

    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_cfg, &m_bus_handle));

    i2c_device_config_t dev_cfg = {};
    dev_cfg.device_address = OLED_ADDR;
    dev_cfg.scl_speed_hz = OLED_I2C_FREQ;

    ESP_ERROR_CHECK(
        i2c_master_bus_add_device(m_bus_handle, &dev_cfg, &m_dev_handle));

    ESP_LOGI(TAG, "I2C master initialized (bus=%p, dev=%p)", m_bus_handle,
             m_dev_handle);
}

void Oled::ping()
{
    uint8_t buf[2] = {0x00, 0x00};
    esp_err_t err = i2c_master_transmit(m_dev_handle, buf, sizeof(buf), -1);

    ESP_LOGI(TAG, "Ping result: %s", esp_err_to_name(err));
}

esp_err_t Oled::sendCmd(uint8_t c)
{
    uint8_t buf[2] = {0x00, c}; // control byte + command
    esp_err_t err = i2c_master_transmit(m_dev_handle, buf, sizeof(buf), -1);

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "sendCmd(0x%02X) failed: %s", c, esp_err_to_name(err));
    }
    return err;
}

esp_err_t Oled::sendData(const uint8_t* data, size_t len)
{
    if (len > SSD1306_WIDTH)
        len = SSD1306_WIDTH;

    uint8_t buf[1 + SSD1306_WIDTH];
    buf[0] = 0x40; // control byte for data
    memcpy(&buf[1], data, len);

    esp_err_t err = i2c_master_transmit(m_dev_handle, buf, 1 + len, -1);

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "sendData(len=%d) failed: %s", (int)len,
                 esp_err_to_name(err));
    }
    return err;
}

void Oled::setPageColumn(std::uint8_t page, std::uint8_t column)
{
    page &= 0x07;
    column &= 0x7F;
    sendCmd(0xB0 | page);
    sendCmd(0x00 | (column & 0x0F));
    sendCmd(0x10 | ((column >> 4) & 0x0F));
}

void Oled::initialize()
{
    sendCmd(0xAE); // Display OFF

    sendCmd(0xD5); // Set Display Clock Divide Ratio / Oscillator Frequency
    sendCmd(0x80); //   Suggested default value

    sendCmd(0xA8); // Set Multiplex Ratio
    sendCmd(0x3F); //   0x3F = 63 → 64 rows (for 128x64 panel)

    sendCmd(0xD3); // Set Display Offset
    sendCmd(0x00); //   No offset

    sendCmd(0x40); // Set Display Start Line = 0

    sendCmd(0x8D); // Charge Pump Setting
    sendCmd(0x14); //   Enable charge pump (0x14 = ON)

    sendCmd(0x20); // Set Memory Addressing Mode
    sendCmd(0x00); //   Horizontal addressing mode

    sendCmd(0xA1); // Set Segment Re-map (mirror horizontally)

    sendCmd(0xC8); // Set COM Output Scan Direction (remapped mode)

    sendCmd(0xDA); // Set COM Pins Hardware Configuration
    sendCmd(0x12); //   Alternative COM pin config, disable left/right remap

    sendCmd(0x81); // Set Contrast Control
    sendCmd(0x7F); //   Medium contrast (0x00–0xFF)

    sendCmd(0xD9); // Set Pre-charge Period
    sendCmd(0xF1); //   Phase 1 = 15, Phase 2 = 1 (recommended)

    sendCmd(0xDB); // Set VCOMH Deselect Level
    sendCmd(0x40); //   ~0.77 × Vcc (recommended)

    sendCmd(0xA4); // Entire Display ON (resume RAM content)

    sendCmd(0xA6); // Set Normal Display (not inverted)

    // Clear display RAM
    std::uint8_t zeros[SSD1306_WIDTH];
    std::memset(zeros, 0x00, sizeof(zeros));

    for (int page = 0; page < SSD1306_PAGES; ++page)
    {
        setPageColumn(page, 0);         // Set page + column to start of line
        sendData(zeros, SSD1306_WIDTH); // Clear one full row
    }

    sendCmd(0xAF); // Display ON
}

} // namespace ssd1306
} // namespace muc