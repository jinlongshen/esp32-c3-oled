#ifndef MUC_OLED_SSD1306_H
#define MUC_OLED_SSD1306_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include "esp_err.h"
#include "II2CDevice.h"
#include "ssd1306_commands.h"

namespace muc
{
namespace ssd1306
{

// -----------------------------------------------------------------------------
// SSD1306 chip geometry (universal, not board-specific)
// -----------------------------------------------------------------------------
constexpr std::size_t SSD1306_WIDTH = 128;
constexpr std::size_t SSD1306_HEIGHT = 64;
constexpr std::size_t SSD1306_PAGES = SSD1306_HEIGHT / 8;

// -----------------------------------------------------------------------------
// Visible area on this board (73x64 at offset 27,24)
//  NOTE: must be defined BEFORE using in std::array
// -----------------------------------------------------------------------------
constexpr std::size_t OLED_WIDTH = 73;
constexpr std::size_t OLED_HEIGHT = 64;
constexpr std::size_t OLED_X_OFFSET = 27;
constexpr std::size_t OLED_Y_OFFSET = 24;

// -----------------------------------------------------------------------------
// OLED driver class
// -----------------------------------------------------------------------------
class Oled
{
  public:
    explicit Oled(II2CDevice& dev) noexcept;
    ~Oled() noexcept = default;

    void drawPixel(int x, int y, bool on) noexcept;
    void clear() noexcept;
    void update() noexcept;

    void drawVisibleBar() noexcept;
    void drawCharA(int x, int y) noexcept;

  private:
    esp_err_t sendCmd(Command c) noexcept;
    esp_err_t sendData(std::span<const std::uint8_t> data) noexcept;

    void setPageColumn(std::uint8_t page, std::uint8_t column) noexcept;
    void initialize() noexcept;

    II2CDevice& m_dev;

    // Framebuffer for the visible OLED region
    std::array<std::uint8_t, OLED_WIDTH * OLED_HEIGHT / 8> m_screen{};
};

} // namespace ssd1306
} // namespace muc

#endif // MUC_OLED_SSD1306_H
