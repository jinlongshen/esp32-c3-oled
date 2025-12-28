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
// Constants: Chip & Board Geometry
// -----------------------------------------------------------------------------
constexpr std::size_t SSD1306_WIDTH = 128;
constexpr std::size_t SSD1306_HEIGHT = 64;
constexpr std::size_t SSD1306_PAGES = SSD1306_HEIGHT / 8;

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
    // Lifecycle
    explicit Oled(II2CDevice& dev) noexcept;
    ~Oled() noexcept = default;

    // Buffer Management
    void clear() noexcept;
    void update() noexcept;

    // High-Level Drawing (Uses m_screen buffer)
    void drawPixel(int x, int y, bool on) noexcept;

    // Direct Hardware Drawing (Bypasses m_screen buffer)
    void drawVisibleBar() noexcept;
    void drawCharA(int x, int y) noexcept;

  private:
    // Internal Hardware Control
    void initialize() noexcept;
    void setPageColumn(std::uint8_t page, std::uint8_t column) noexcept;

    // Hardware I/O
    esp_err_t sendCmd(Command c) noexcept;
    esp_err_t sendData(std::span<const std::uint8_t> data) noexcept;

    // Members
    II2CDevice& m_dev;

    // Framebuffer for the visible OLED region
    std::array<std::uint8_t, OLED_WIDTH * OLED_HEIGHT / 8> m_screen;
};

} // namespace ssd1306
} // namespace muc

#endif // MUC_OLED_SSD1306_H