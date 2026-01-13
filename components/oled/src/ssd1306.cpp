#include "ssd1306.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>

#include <esp_log.h>

namespace
{
constexpr const char* TAG = "OLED_SSD1306_GEOM";
}

namespace muc
{
namespace ssd1306
{

// =============================================================================
// Lifecycle (Constructor & Initialization)
// =============================================================================

Oled::Oled(II2CDevice& dev, const DisplayGeometry& g) noexcept
: m_dev(dev)
, m_geometry(g)
, m_screen{}
{
    initialize();
}

void Oled::initialize() noexcept
{
    using C = Command;

    struct CommandStep
    {
        C cmd;
        std::uint8_t param;
        bool has_param;
    };

    // Basic SSD1306 init sequence; use RAM geometry where it matters
    static constexpr std::array<CommandStep, 14> init_steps = {{
        {C::DisplayOff, 0x00, false},
        {C::SetClockDiv, 0x80, true},
        // Multiplex ratio: (ram_height - 1) for 128x64 (0x3F), but generic
        {C::SetMultiplex, 0x3F, true}, // overwritten below with real value
        {C::SetDisplayOffset, 0x00, true},
        {C::SetStartLine, 0x00, false},
        {C::ChargePump, C::ChargePumpOn, true},
        {C::MemoryMode, C::MemoryModeHorizontal, true},
        {C::SegmentRemap, 0x00, false},
        {C::ComScanDec, 0x00, false},
        {C::SetComPins, 0x12, true},
        {C::SetContrast, 0x7F, true},
        {C::SetPrecharge, 0xF1, true},
        {C::SetVcomDetect, 0x40, true},
        {C::ResumeRAM, 0x00, false},
    }};

    // Run init steps, but override SetMultiplex parameter with m_geometry.ram_height - 1
    for (const auto& step : init_steps)
    {
        (void)sendCmd(step.cmd);

        if (step.has_param)
        {
            std::uint8_t param = step.param;
            if (step.cmd == C::SetMultiplex)
            {
                param = static_cast<std::uint8_t>(m_geometry.ram_height - 1);
            }

            std::array<std::uint8_t, 2> buf = {0x00, param};
            (void)m_dev.write(std::span<const std::uint8_t>(buf.data(), buf.size()));
        }
    }

    // Turn display on after RAM clear
    // Clear entire display RAM (full ram_width × ram_pages) with zeros.
    // We bypass setPageColumn() here to ignore visible offsets and clear full RAM.
    std::array<std::uint8_t, SSD1306_WIDTH> zeros{};
    for (int page = 0; page < m_geometry.ram_pages; ++page)
    {
        // Set page
        sendCmd(static_cast<C>(0xB0 | (page & 0x07)));

        // Set column to 0 (no offset here, full chip RAM)
        sendCmd(static_cast<C>(0x00 | 0x00)); // lower 4 bits
        sendCmd(static_cast<C>(0x10 | 0x00)); // upper 4 bits

        // Send as many zeros as the chip width supports (clamped in sendData)
        (void)sendData(std::span<const std::uint8_t>(zeros.data(), m_geometry.ram_width));
    }

    (void)sendCmd(C::DisplayOn);
}

// =============================================================================
// High-Level Drawing (Uses m_screen buffer)
// =============================================================================

void Oled::drawPixel(int x, int y, bool on) noexcept
{
    // Logical coordinates in the visible window: x ∈ [0, width), y ∈ [0, height)
    if (x < 0 || x >= m_geometry.width || y < 0 || y >= m_geometry.height)
    {
        return;
    }

    int page = y / 8;
    int bit_in_page = y % 8;
    int index = page * m_geometry.width + x;

    std::uint8_t mask = static_cast<std::uint8_t>(1u << bit_in_page);

    if (on)
        m_screen[index] |= mask;
    else
        m_screen[index] &= static_cast<std::uint8_t>(~mask);
}

// LVGL 1‑bit buffer (width × height) → page‑tiled m_screen.
//
// Assumptions (same as your original):
//  - lvbuf is a 1‑bit, row‑major buffer: 8 pixels per byte
//  - bit order: MSB‑first (bit 7 = leftmost pixel in the byte) if LSB_FIRST == false
//  - LVGL display size matches m_geometry.width × m_geometry.height
void Oled::blitLVGLBuffer(std::span<const std::uint8_t> lvbuf) noexcept
{
    const std::size_t expected = static_cast<std::size_t>(m_geometry.width * m_geometry.height) / 8;

    if (lvbuf.size() < expected)
    {
        ESP_LOGE(TAG,
                 "blitLVGLBuffer: buffer too small, got %u, expected %u",
                 static_cast<unsigned>(lvbuf.size()),
                 static_cast<unsigned>(expected));
        return;
    }

    m_screen.fill(0);

    for (int y = 0; y < m_geometry.height; ++y)
    {
        for (int x = 0; x < m_geometry.width; ++x)
        {
            int bit_index = y * m_geometry.width + x;
            int byte_index = bit_index >> 3;
            int bit_in_byte = 0;

            // LVGL I1 (1‑bit) format stores 8 horizontal pixels per byte.
            // Pixel order inside each byte is MSB‑first:
            //
            //     bit 7  bit 6  bit 5  bit 4  bit 3  bit 2  bit 1  bit 0
            //     px0    px1    px2    px3    px4    px5    px6    px7
            //
            // Meaning:
            //   - The leftmost pixel in the group of 8 is stored in bit 7.
            //   - The rightmost pixel in the group of 8 is stored in bit 0.
            //
            // `bit_index` is the absolute pixel index in the LVGL buffer.
            // `(bit_index & 7)` gives the pixel’s position *within its byte* (0–7).
            //
            // To convert this pixel position to the correct MSB‑first bit position,
            // we invert it:   MSB_position = 7 - pixel_position.
            //
            // Example:
            //   bit_index = 13
            //   pixel_position = 13 & 7 = 5
            //   bit_in_byte   = 7 - 5 = 2   → pixel is stored in bit 2 of its byte.
            //
            // Final mapping:
            bit_in_byte = 7 - (bit_index & 7); // MSB‑first pixel‑to‑bit mapping

            bool on = (lvbuf[byte_index] >> bit_in_byte) & 0x1;
            if (!on)
            {
                continue;
            }

            int page = y / 8;
            int bit_in_page = y % 8;
            int dst_index = page * m_geometry.width + x;

            m_screen[dst_index] |= static_cast<std::uint8_t>(1u << bit_in_page);
        }
    }
}

// =============================================================================
// Buffer Management
// =============================================================================

void Oled::clear() noexcept
{
    m_screen.fill(0);
}

void Oled::update() noexcept
{
    int pages = m_geometry.height / 8;

    for (int p = 0; p < pages; ++p)
    {
        // Physical page in SSD1306 address space (apply Y offset here)
        std::uint8_t phys_page = static_cast<std::uint8_t>(p + (m_geometry.y_offset / 8));

        // Move column pointer to the start of the visible window (local column 0)
        setPageColumn(phys_page, 0);

        // Start of this page in m_screen (local width-byte strip)
        const std::uint8_t* row_ptr = m_screen.data() + (p * m_geometry.width);

        // Send the visible width bytes for this horizontal strip
        sendData(
            std::span<const std::uint8_t>(row_ptr, static_cast<std::size_t>(m_geometry.width)));
    }
}

// =============================================================================
// Low-Level Hardware I/O
// =============================================================================

esp_err_t Oled::sendCmd(Command c) noexcept
{
    std::uint8_t buf[2] = {0x00, static_cast<std::uint8_t>(c)};
    return m_dev.write(std::span<const std::uint8_t>(buf, 2));
}

esp_err_t Oled::sendData(std::span<const std::uint8_t> data) noexcept
{
    // Clamp to the chip-level width
    if (data.size() > SSD1306_WIDTH)
    {
        data = data.first(SSD1306_WIDTH);
    }

    std::array<std::uint8_t, 1 + SSD1306_WIDTH> buf{};
    buf[0] = 0x40;
    std::copy(data.begin(), data.end(), buf.begin() + 1);

    auto err = m_dev.write(std::span<const std::uint8_t>(buf.data(), 1 + data.size()));

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "sendData failed: %s", esp_err_to_name(err));
    }
    return err;
}

void Oled::setPageColumn(std::uint8_t page, std::uint8_t column) noexcept
{
    // SSD1306 has 8 pages (0–7)
    page &= 0x07;

    // Logical columns are 0–127
    column &= 0x7F;

    // Map local column to hardware: apply X offset
    std::uint8_t hw_column = static_cast<std::uint8_t>(column + m_geometry.x_offset);

    // Set page
    sendCmd(static_cast<Command>(0xB0 | page));
    // Set lower 4 bits of column
    sendCmd(static_cast<Command>(0x00 | (hw_column & 0x0F)));
    // Set upper 4 bits of column
    sendCmd(static_cast<Command>(0x10 | ((hw_column >> 4) & 0x0F)));

    ESP_LOGD(TAG, "setPageColumn: page=%u column=%u hw_column=%u", page, column, hw_column);
}

void Oled::set_scan_mode(bool enable) noexcept
{

    if (enable)
    {
        // 1. Lower Contrast: reduces "blooming/glow" for the camera
        (void)sendCmd(Command::SetContrast);
        std::array<std::uint8_t, 2> contrast_buf = {0x00,
                                                    0x30}; // 0x00 = control byte, 0x30 = value
        (void)m_dev.write(std::span<const std::uint8_t>(contrast_buf.data(), 2));

        // 2. Increase Osc Frequency: reduces "rolling black bars" in video
        (void)sendCmd(Command::SetClockDiv);
        std::array<std::uint8_t, 2> clock_buf = {0x00, 0xF0}; // 0xF0 = Max freq
        (void)m_dev.write(std::span<const std::uint8_t>(clock_buf.data(), 2));
    }
    else
    {
        // Restore to normal viewing settings (Defaults from your initialize() function)
        (void)sendCmd(Command::SetContrast);
        std::array<std::uint8_t, 2> contrast_buf = {0x00, 0x7F};
        (void)m_dev.write(std::span<const std::uint8_t>(contrast_buf.data(), 2));

        (void)sendCmd(Command::SetClockDiv);
        std::array<std::uint8_t, 2> clock_buf = {0x00, 0x80};
        (void)m_dev.write(std::span<const std::uint8_t>(clock_buf.data(), 2));
    }
}

} // namespace ssd1306
} // namespace muc
