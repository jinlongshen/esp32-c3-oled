#ifndef COMPONENTS_OLED_SSD1306_H
#define COMPONENTS_OLED_SSD1306_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include <esp_err.h>

#include "II2CDevice.h"
#include "display_geometry.h"
#include "ssd1306_commands.h"

namespace muc
{
namespace ssd1306
{

// Chip-level SSD1306 RAM geometry (not glass geometry)
constexpr std::size_t SSD1306_WIDTH = 128;
constexpr std::size_t SSD1306_HEIGHT = 64;
constexpr std::size_t SSD1306_PAGES = SSD1306_HEIGHT / 8;

constexpr std::uint8_t OLED_ADDR = 0x3C;

class Oled
{
  public:
    explicit Oled(II2CDevice& dev, const DisplayGeometry& g) noexcept;
    ~Oled() noexcept = default;

    // LVGL → local framebuffer (buffer must match geometry width/height/format)
    void blitLVGLBuffer(std::span<const std::uint8_t> lvbuf) noexcept;

    // Optional helpers for ad‑hoc drawing/debugging
    void clear() noexcept;
    void drawPixel(int x, int y, bool on) noexcept;

    // Push local framebuffer to the physical display
    void update() noexcept;

    // Read-only access to geometry
    const DisplayGeometry& geometry() const noexcept
    {
        return m_geometry;
    }

    void set_scan_mode(bool enable) noexcept;

  private:
    void initialize() noexcept;
    void setPageColumn(std::uint8_t page, std::uint8_t column) noexcept;

    esp_err_t sendCmd(Command c) noexcept;
    esp_err_t sendData(std::span<const std::uint8_t> data) noexcept;

  private:
    II2CDevice& m_dev;
    DisplayGeometry m_geometry;

    // Full SSD1306 RAM buffer: 128×64 / 8 = 1024 bytes
    // Only the first (width * height / 8) bytes are used for the visible window.
    std::array<std::uint8_t, SSD1306_WIDTH * SSD1306_HEIGHT / 8> m_screen{};
};

} // namespace ssd1306
} // namespace muc

#endif // COMPONENTS_OLED_SSD1306_H
