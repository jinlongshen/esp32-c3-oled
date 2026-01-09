#ifndef COMPONENTS_OLED_SSD1306_H
#define COMPONENTS_OLED_SSD1306_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "II2CDevice.h"
#include "ssd1306_commands.h"

namespace muc
{
namespace ssd1306
{

constexpr std::uint8_t OLED_ADDR = 0x3C;

// -----------------------------------------------------------------------------
// Constants: Chip & Board Geometry
// -----------------------------------------------------------------------------
constexpr std::size_t SSD1306_WIDTH = 128;
constexpr std::size_t SSD1306_HEIGHT = 64;
constexpr std::size_t SSD1306_PAGES = SSD1306_HEIGHT / 8;

// Visible window on your 72×40 OLED
constexpr std::size_t OLED_WIDTH = 72;
constexpr std::size_t OLED_HEIGHT = 40;
constexpr std::size_t OLED_PAGES = OLED_HEIGHT / 8;
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

    // Background task (optional, not required for backward compatibility)
    void startTask() noexcept;

    // LVGL → local framebuffer
    void blitLVGLBuffer(std::span<const std::uint8_t> lvbuf) noexcept;

    // Called from LVGL flush callback to wake OLED task
    void notifyFrameReady() noexcept;

    // Optional helpers for ad‑hoc drawing/debugging
    void clear() noexcept;
    void drawPixel(int x, int y, bool on) noexcept;

    // -------------------------------------------------------------------------
    // IMPORTANT: update() is PUBLIC again for backward compatibility
    // -------------------------------------------------------------------------
    void update() noexcept;

  private:
    // Background task loop (only used if startTask() is used)
    void taskLoop() noexcept;

    // Internal Hardware Control
    void initialize() noexcept;
    void setPageColumn(std::uint8_t page, std::uint8_t column) noexcept;

    // Hardware I/O
    esp_err_t sendCmd(Command c) noexcept;
    esp_err_t sendData(std::span<const std::uint8_t> data) noexcept;

  private:
    II2CDevice& m_dev;

    // Framebuffer for the visible OLED region
    std::array<std::uint8_t, OLED_WIDTH * OLED_HEIGHT / 8> m_screen;

    // Task + semaphore (unused unless startTask() is used)
    TaskHandle_t m_task = nullptr;
    SemaphoreHandle_t m_frame_sem = nullptr;
};

} // namespace ssd1306
} // namespace muc

#endif // COMPONENTS_OLED_SSD1306_H
