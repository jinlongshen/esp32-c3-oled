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

Oled::Oled(II2CDevice& dev) noexcept
: m_dev(dev)
{
    initialize();
}

esp_err_t Oled::sendCmd(Command c) noexcept
{
    std::uint8_t buf[2] = {
        0x00, static_cast<std::uint8_t>(c)}; // control byte + command
    return m_dev.write(std::span<const std::uint8_t>(buf, 2));
}

esp_err_t Oled::sendData(std::span<const std::uint8_t> data) noexcept
{
    if (data.size() > SSD1306_WIDTH)
    {
        data = data.first(SSD1306_WIDTH);
    }

    std::array<std::uint8_t, 1 + SSD1306_WIDTH> buf{};
    buf[0] = 0x40; // control byte for data
    std::copy(data.begin(), data.end(), buf.begin() + 1);

    auto err =
        m_dev.write(std::span<const std::uint8_t>(buf.data(), 1 + data.size()));
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG,
                 "sendData(len=%u) failed: %s",
                 static_cast<unsigned>(data.size()),
                 esp_err_to_name(err));
    }
    return err;
}

void Oled::setPageColumn(std::uint8_t page, std::uint8_t column) noexcept
{
    page &= 0x07;
    column &= 0x7F;

    sendCmd(static_cast<Command>(0xB0 | page));                   // set page
    sendCmd(static_cast<Command>(0x00 | (column & 0x0F)));        // low col
    sendCmd(static_cast<Command>(0x10 | ((column >> 4) & 0x0F))); // high col
}

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

void Oled::drawCharA(int x, int y) noexcept
{
    // Simple 5x8 'A' bitmap, column-major, LSB at top
    constexpr std::array<std::uint8_t, 5> A_5x8 = {
        0x7C, 0x12, 0x11, 0x12, 0x7C};

    const int page = (OLED_Y_OFFSET + y) / 8;
    const int column = OLED_X_OFFSET + x;

    setPageColumn(static_cast<std::uint8_t>(page),
                  static_cast<std::uint8_t>(column));

    sendData(A_5x8);
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

        {C::SetClockDiv, 0x80, true},  // suggested default
        {C::SetMultiplex, 0x3F, true}, // 63 -> 64 rows

        {C::SetDisplayOffset, 0x00, true}, // no offset
        {C::SetStartLine, 0x00, false},    // already encoded in cmd

        {C::ChargePump, 0x14, true}, // enable charge pump
        {C::MemoryMode, 0x00, true}, // horizontal addressing

        {C::SegmentRemap, 0x00, false}, // A1
        {C::ComScanDec, 0x00, false},   // C8

        {C::SetComPins, 0x12, true},  // alt COM config
        {C::SetContrast, 0x7F, true}, // medium contrast

        {C::SetPrecharge, 0xF1, true},  // phase 1=15, phase 2=1
        {C::SetVcomDetect, 0x40, true}, // ~0.77 * Vcc

        {C::ResumeRAM, 0x00, false},     // A4
        {C::NormalDisplay, 0x00, false}, // A6
    }};

    // Send initialization sequence
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

void Oled::clear() noexcept
{
    m_screen.fill(0);
    update();
}

void Oled::update() noexcept
{
    // Push only the visible 73x64 window starting at OLED_X_OFFSET
    for (int page = 0; page < static_cast<int>(OLED_HEIGHT / 8); ++page)
    {
        setPageColumn(static_cast<std::uint8_t>(page),
                      static_cast<std::uint8_t>(OLED_X_OFFSET));

        auto* start = m_screen.data() + page * static_cast<int>(OLED_WIDTH);
        std::span<const std::uint8_t> data(start, OLED_WIDTH);

        sendData(data);
    }
}

} // namespace ssd1306
} // namespace muc
