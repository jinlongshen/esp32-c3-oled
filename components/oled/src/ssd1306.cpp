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

Oled::Oled() noexcept
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
    ESP_LOGI(TAG,
             "Starting SSD1306 test for Abrobot 0.42\" OLED (classic I2C)");
    i2c_config_t conf = {};
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = OLED_SDA_PIN;
    conf.scl_io_num = OLED_SCL_PIN;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = OLED_I2C_FREQ;
#if SOC_I2C_SUPPORT_REF_TICK
    conf.clk_flags = 0; // use default clock source
#endif
    ESP_ERROR_CHECK(i2c_param_config(s_OLED_I2C_PORT, &conf));
    ESP_ERROR_CHECK(i2c_driver_install(s_OLED_I2C_PORT, conf.mode, 0, 0, 0));
    ESP_LOGI(TAG, "I2C master initialized on port %d", s_OLED_I2C_PORT);
}

void Oled::ping()
{
    std::uint8_t ping_buf[2] = {0x00, 0x00};
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    esp_err_t ping_err = ESP_OK;
    ping_err |= i2c_master_start(cmd);
    ping_err |=
        i2c_master_write_byte(cmd, (OLED_ADDR << 1) | I2C_MASTER_WRITE, true);
    ping_err |= i2c_master_write(cmd, ping_buf, sizeof(ping_buf), true);
    ping_err |= i2c_master_stop(cmd);
    if (ping_err == ESP_OK)
    {
        ping_err = i2c_master_cmd_begin(s_OLED_I2C_PORT, cmd,
                                        100 / portTICK_PERIOD_MS);
    }
    i2c_cmd_link_delete(cmd);
    ESP_LOGI(TAG, "Ping transmit result: %s", esp_err_to_name(ping_err));
}

esp_err_t Oled::sendCmd(std::uint8_t c)
{
    std::uint8_t buf[2] = {0x00, c};
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    if (!cmd)
    {
        return ESP_FAIL;
    }
    esp_err_t err = ESP_OK;
    err |= i2c_master_start(cmd);
    err |=
        i2c_master_write_byte(cmd, (OLED_ADDR << 1) | I2C_MASTER_WRITE, true);
    err |= i2c_master_write(cmd, buf, sizeof(buf), true);
    err |= i2c_master_stop(cmd);
    if (err == ESP_OK)
    {
        err = i2c_master_cmd_begin(s_OLED_I2C_PORT, cmd,
                                   100 / portTICK_PERIOD_MS);
    }
    i2c_cmd_link_delete(cmd);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "cmd 0x%02X failed: %s", c, esp_err_to_name(err));
    }
    return err;
}

esp_err_t Oled::sendData(const std::uint8_t* data, std::size_t len)
{
    if (len > SSD1306_WIDTH)
    {
        len = SSD1306_WIDTH;
    }
    std::uint8_t buf[1 + SSD1306_WIDTH];
    buf[0] = 0x40;
    std::memcpy(&buf[1], data, len);
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    if (!cmd)
    {
        return ESP_FAIL;
    }
    esp_err_t err = ESP_OK;
    err |= i2c_master_start(cmd);
    err |=
        i2c_master_write_byte(cmd, (OLED_ADDR << 1) | I2C_MASTER_WRITE, true);
    err |= i2c_master_write(cmd, buf, 1 + len, true);
    err |= i2c_master_stop(cmd);
    if (err == ESP_OK)
    {
        err = i2c_master_cmd_begin(s_OLED_I2C_PORT, cmd,
                                   100 / portTICK_PERIOD_MS);
    }
    i2c_cmd_link_delete(cmd);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "data len %d failed: %s", (int)len, esp_err_to_name(err));
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
    sendCmd(0xAE);
    sendCmd(0xD5);
    sendCmd(0x80);
    sendCmd(0xA8);
    sendCmd(0x3F);
    sendCmd(0xD3);
    sendCmd(0x00);
    sendCmd(0x40);
    sendCmd(0x8D);
    sendCmd(0x14);
    sendCmd(0x20);
    sendCmd(0x00);
    sendCmd(0xA1);
    sendCmd(0xC8);
    sendCmd(0xDA);
    sendCmd(0x12);
    sendCmd(0x81);
    sendCmd(0x7F);
    sendCmd(0xD9);
    sendCmd(0xF1);
    sendCmd(0xDB);
    sendCmd(0x40);
    sendCmd(0xA4);
    sendCmd(0xA6);
    std::uint8_t zeros[SSD1306_WIDTH];
    std::memset(zeros, 0x00, sizeof(zeros));
    for (int page = 0; page < SSD1306_PAGES; ++page)
    {
        setPageColumn(page, 0);
        sendData(zeros, SSD1306_WIDTH);
    }
    sendCmd(0xAF);
}

} // namespace ssd1306
} // namespace muc