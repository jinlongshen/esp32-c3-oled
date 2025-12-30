#include "ssd1306.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>

#include "esp_log.h"

namespace
{
constexpr const char* TAG = "OLED_SSD1306_ABROBOT";
}

namespace muc
{
namespace ssd1306
{

// =============================================================================
// Lifecycle (Constructor & Initialization)
// =============================================================================

Oled::Oled(II2CDevice& dev) noexcept
: m_dev(dev)
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

    static constexpr std::array<CommandStep, 15> init_steps = {{
        {C::DisplayOff, 0x00, false},
        {C::SetClockDiv, 0x80, true},
        {C::SetMultiplex, 0x3F, true},
        {C::SetDisplayOffset, 0x00, true},
        {C::SetStartLine, 0x00, false},
        {C::ChargePump, 0x14, true},
        {C::MemoryMode, 0x00, true},
        {C::SegmentRemap, 0x00, false},
        {C::ComScanDec, 0x00, false},
        {C::SetComPins, 0x12, true},
        {C::SetContrast, 0x7F, true},
        {C::SetPrecharge, 0xF1, true},
        {C::SetVcomDetect, 0x40, true},
        {C::ResumeRAM, 0x00, false},
        {C::NormalDisplay, 0x00, false},
    }};

    for (const auto& step : init_steps)
    {
        (void)sendCmd(step.cmd);

        if (step.has_param)
        {
            std::array<std::uint8_t, 2> buf = {0x00, step.param};
            (void)m_dev.write(
                std::span<const std::uint8_t>(buf.data(), buf.size()));
        }
    }

    // Clear entire display RAM (full 128x64) with zeros
    std::array<std::uint8_t, SSD1306_WIDTH> zeros{};
    for (int page = 0; page < static_cast<int>(SSD1306_PAGES); ++page)
    {
        setPageColumn(static_cast<std::uint8_t>(page), 0);
        (void)sendData(zeros);
    }

    (void)sendCmd(C::DisplayOn);
}

// =============================================================================
// High-Level Drawing (Uses m_screen buffer)
// =============================================================================

void Oled::drawPixel(int x, int y, bool on) noexcept
{
    // Map logical (x, y) to framebuffer-local coordinates
    int fx = x - static_cast<int>(OLED_X_OFFSET);
    int fy = y - static_cast<int>(OLED_Y_OFFSET);

    if (fx < 0 || fx >= static_cast<int>(OLED_WIDTH) || fy < 0 ||
        fy >= static_cast<int>(OLED_HEIGHT))
        return;

    int index = fx + (fy / 8) * static_cast<int>(OLED_WIDTH);
    uint8_t mask = 1 << (fy % 8);

    if (on)
        m_screen[index] |= mask;
    else
        m_screen[index] &= ~mask;
}

// =============================================================================
// Direct Hardware Drawing (Bypasses m_screen buffer)
// =============================================================================

void Oled::drawVisibleBar() noexcept
{
    std::uint8_t page = (OLED_Y_OFFSET / 8);
    std::array<std::uint8_t, 32> bar;
    bar.fill(0xFF);

    setPageColumn(page, OLED_X_OFFSET);
    sendData(bar);

    ESP_LOGI(TAG,
             "Drew white bar at x=%d..%d, page=%d (y≈%d..%d)",
             static_cast<int>(OLED_X_OFFSET),
             static_cast<int>(OLED_X_OFFSET + bar.size() - 1),
             static_cast<int>(page),
             static_cast<int>(page * 8),
             static_cast<int>(page * 8 + 7));
}

// void Oled::drawCharA(int x, int y) noexcept
// {
//     constexpr std::array<std::uint8_t, 5> A_5x8 = {
//         0x7C, 0x12, 0x11, 0x12, 0x7C};

//     const int page = (OLED_Y_OFFSET + y) / 8;
//     const int column = OLED_X_OFFSET + x;

//     setPageColumn(static_cast<std::uint8_t>(page),
//                   static_cast<std::uint8_t>(column));

//     sendData(A_5x8);
// }

// void Oled::drawCharA(int x, int y) noexcept
// {
//     static constexpr std::array<std::uint8_t, 5> A_5x8 = {
//         0x7C, 0x12, 0x11, 0x12, 0x7C
//     };

//     // Calculate which logical page (0-7) the 'y' coordinate falls into
//     int logical_page = y / 8;

//     // Ensure we are within vertical bounds of the buffer
//     if (logical_page < 0 || logical_page >= static_cast<int>(OLED_PAGES)) {
//         return;
//     }

//     // Find the starting position in m_screen for this specific page and x-offset
//     // m_screen is a flat array: [Page0: 0..72][Page1: 73..145]...
//     std::size_t buffer_offset = (logical_page * OLED_WIDTH) + x;

//     for (std::size_t i = 0; i < A_5x8.size(); ++i)
//     {
//         // Check horizontal bounds for each column of the character
//         if ((x + i) < OLED_WIDTH)
//         {
//             // Direct byte assignment: each byte in A_5x8 replaces
//             // the 8 vertical pixels at m_screen[index]
//             m_screen[buffer_offset + i] = A_5x8[i];
//         }
//     }
// }

void Oled::drawCharA(int x, int y) noexcept
{
    static constexpr std::array<std::uint8_t, 5> A_5x8 = {
        0x7C, 0x12, 0x11, 0x12, 0x7C};

    // 1. Map global (x, y) to local framebuffer (fx, fy)
    int fx = x - static_cast<int>(OLED_X_OFFSET);
    int fy = y - static_cast<int>(OLED_Y_OFFSET);

    // 2. Vertical Bounds Check
    // A 5x8 font fits exactly in one page IF fy is a multiple of 8.
    int logical_page = fy / 8;
    if (logical_page < 0 || logical_page >= static_cast<int>(OLED_PAGES) ||
        fy % 8 != 0)
    {
        // Note: This logic only works perfectly if y is aligned to a page (0, 8, 16...)
        return;
    }

    // 3. Calculate Buffer Start
    std::size_t buffer_offset = (logical_page * OLED_WIDTH) + fx;

    for (std::size_t i = 0; i < A_5x8.size(); ++i)
    {
        // 4. Horizontal Bounds Check (using local fx)
        int current_fx = fx + static_cast<int>(i);
        if (current_fx >= 0 && current_fx < static_cast<int>(OLED_WIDTH))
        {
            m_screen[buffer_offset + i] = A_5x8[i];
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
    // OLED_PAGES is 8 (representing your 64-pixel height)
    for (std::size_t p = 0; p < OLED_PAGES; ++p)
    {
        // Calculate physical page: 0+3=3, 1+3=4, ... 7+3=10
        uint8_t phys_page = static_cast<uint8_t>(p + (OLED_Y_OFFSET / 8));

        // Always reset the 'pen' to the start of the visible column (27 or 28)
        // This makes sure each row is perfectly aligned.
        setPageColumn(phys_page, OLED_X_OFFSET);

        // Calculate the start of this page in your flat 1D m_screen array
        // Page 0: index 0, Page 1: index 73, etc.
        const std::uint8_t* row_ptr = m_screen.data() + (p * OLED_WIDTH);

        // Send the 73 vertical-bytes for this horizontal strip
        sendData(std::span<const std::uint8_t>(row_ptr, OLED_WIDTH));
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
    if (data.size() > SSD1306_WIDTH)
    {
        data = data.first(SSD1306_WIDTH);
    }

    std::array<std::uint8_t, 1 + SSD1306_WIDTH> buf{};
    buf[0] = 0x40;
    std::copy(data.begin(), data.end(), buf.begin() + 1);

    auto err =
        m_dev.write(std::span<const std::uint8_t>(buf.data(), 1 + data.size()));
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "sendData failed: %s", esp_err_to_name(err));
    }
    return err;
}

void Oled::setPageColumn(std::uint8_t page, std::uint8_t column) noexcept
{
    page &= 0x07;
    column &= 0x7F;

    sendCmd(static_cast<Command>(0xB0 | page));
    sendCmd(static_cast<Command>(0x00 | (column & 0x0F)));
    sendCmd(static_cast<Command>(0x10 | ((column >> 4) & 0x0F)));
}

} // namespace ssd1306
} // namespace muc