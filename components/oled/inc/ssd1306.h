#ifndef MUC_OLED_SSD1306_H
#define MUC_OLED_SSD1306_H

#include <cstddef>
#include <cstdint>

#include "driver/gpio.h"
#include "driver/i2c.h"
#include "esp_err.h"

namespace muc
{
namespace ssd1306
{
class Oled
{
  public:
    Oled() noexcept;
    ~Oled() noexcept = default;
    void drawVisibleBar() noexcept;
    void drawCharA(int x, int y) noexcept;

  private:
    void initI2C() noexcept;
    void ping() noexcept;
    esp_err_t sendCmd(std::uint8_t c) noexcept;
    esp_err_t sendData(const std::uint8_t* data, std::size_t length) noexcept;
    void setPageColumn(std::uint8_t page, std::uint8_t column) noexcept;
    void initialize() noexcept;

    // Board pins (Abrobot ESP32-C3 0.42" OLED)
    static constexpr gpio_num_t OLED_SDA_PIN = GPIO_NUM_5;
    static constexpr gpio_num_t OLED_SCL_PIN = GPIO_NUM_6;
    static constexpr std::uint32_t OLED_I2C_FREQ = 400000;
    static constexpr std::uint8_t OLED_ADDR = 0x3C;
    static constexpr i2c_port_t s_OLED_I2C_PORT = I2C_NUM_0;

    // for this ESP32-C3 AbRobot board: visible area 73x64 at x + 27 and y + 24
    static constexpr std::size_t OLED_WIDTH = 73;
    static constexpr std::size_t OLED_HEIGHT = 64;
    static constexpr std::size_t OLED_X_OFFSET = 27;
    static constexpr std::size_t OLED_Y_OFFSET = 24;

    // SSD1306 internal buffer size
    static constexpr std::size_t SSD1306_WIDTH = 128;
    static constexpr std::size_t SSD1306_HEIGHT = 64;
    static constexpr std::size_t SSD1306_PAGES = (SSD1306_HEIGHT / 8);
};
} // namespace ssd1306
} // namespace muc

#endif // #define MUC_OLED_SSD1306_H
