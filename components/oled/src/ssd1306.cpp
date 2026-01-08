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
    m_frame_sem = xSemaphoreCreateBinary();
    if (!m_frame_sem)
    {
        ESP_LOGE(TAG, "Failed to create frame semaphore");
        return;
    }

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

    // Basic SSD1306 init sequence for 128x64 panel
    static constexpr std::array<CommandStep, 15> init_steps = {{
        {C::DisplayOff, 0x00, false},
        {C::SetClockDiv, 0x80, true},
        {C::SetMultiplex, 0x3F, true}, // 1/64 duty
        {C::SetDisplayOffset, 0x00, true},
        {C::SetStartLine, 0x00, false},
        {C::ChargePump, C::ChargePumpOn, true},
        {C::MemoryMode, C::MemoryModeHorizontal, true},
        {C::SegmentRemap, 0x00, false}, // 0xA1 already encoded in enum
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
            (void)m_dev.write(std::span<const std::uint8_t>(buf.data(), buf.size()));
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
// Task Management (decouple LVGL from I2C)
// =============================================================================

void Oled::startTask() noexcept
{
    if (!m_frame_sem)
    {
        ESP_LOGE(TAG, "Cannot start OLED task, semaphore not created");
        return;
    }

    if (m_task)
    {
        return; // already running
    }

    BaseType_t ok = xTaskCreate([](void* arg) { static_cast<Oled*>(arg)->taskLoop(); },
                                "oled_task",
                                4096, // adjust after measuring HWM
                                this,
                                4, // priority: same or lower than lvgl handler task
                                &m_task);

    if (ok != pdPASS)
    {
        ESP_LOGE(TAG, "Failed to create OLED task");
        m_task = nullptr;
    }
}

void Oled::taskLoop() noexcept
{
    ESP_LOGI(TAG, "OLED task started");

    for (;;)
    {
        // Wait until LVGL has produced a new frame
        if (xSemaphoreTake(m_frame_sem, portMAX_DELAY) == pdTRUE)
        {
            // Now it's safe to perform blocking I2C transfers
            update();
        }
    }
}

void Oled::notifyFrameReady() noexcept
{
    if (!m_frame_sem)
    {
        return;
    }

    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    if (xPortInIsrContext())
    {
        xSemaphoreGiveFromISR(m_frame_sem, &xHigherPriorityTaskWoken);
        if (xHigherPriorityTaskWoken == pdTRUE)
        {
            portYIELD_FROM_ISR();
        }
    }
    else
    {
        xSemaphoreGive(m_frame_sem);
    }
}

// =============================================================================
// High-Level Drawing (Uses m_screen buffer)
// =============================================================================

// Optional helper for ad-hoc drawing/debugging.
// Coordinates are in the logical 72x40 window (with offsets applied here).
void Oled::drawPixel(int x, int y, bool on) noexcept
{
    // Map logical (x, y) to framebuffer-local coordinates
    int fx = x - static_cast<int>(OLED_X_OFFSET);
    int fy = y - static_cast<int>(OLED_Y_OFFSET);

    if (fx < 0 || fx >= static_cast<int>(OLED_WIDTH) || fy < 0 ||
        fy >= static_cast<int>(OLED_HEIGHT))
    {
        return;
    }

    int index = fx + (fy / 8) * static_cast<int>(OLED_WIDTH);
    std::uint8_t mask = static_cast<std::uint8_t>(1u << (fy % 8));

    if (on)
        m_screen[index] |= mask;
    else
        m_screen[index] &= static_cast<std::uint8_t>(~mask);
}

// LVGL 1‑bit buffer (VISIBLE_W x VISIBLE_H) → 72x40 page‑tiled m_screen.
//
// Assumptions:
//  - lvbuf is a 1‑bit, row‑major buffer: 8 pixels per byte
//  - bit order: MSB‑first (bit 7 = leftmost pixel in the byte)
//  - LVGL display size matches OLED_WIDTH x OLED_HEIGHT
void Oled::blitLVGLBuffer(std::span<const std::uint8_t> lvbuf) noexcept
{
    const std::size_t expected = (OLED_WIDTH * OLED_HEIGHT) / 8;
    if (lvbuf.size() < expected)
    {
        return;
    }

    m_screen.fill(0);

    for (int y = 0; y < static_cast<int>(OLED_HEIGHT); ++y)
    {
        for (int x = 0; x < static_cast<int>(OLED_WIDTH); ++x)
        {
            int bit_index = y * static_cast<int>(OLED_WIDTH) + x;
            int byte_index = bit_index >> 3;
            int bit_in_byte = 7 - (bit_index & 7);

            bool on = (lvbuf[byte_index] >> bit_in_byte) & 0x1;
            if (!on)
            {
                continue;
            }

            int page = y / 8;
            int bit_in_page = y % 8;
            int dst_index = page * static_cast<int>(OLED_WIDTH) + x;

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

// Push logical 72x40 window (m_screen) into the correct 128x64 SSD1306 address
// space using OLED_X_OFFSET and OLED_Y_OFFSET.
//
// This is blocking and must only be called from the OLED task.
void Oled::update() noexcept
{
    for (std::size_t p = 0; p < OLED_PAGES; ++p)
    {
        // Physical page in SSD1306 address space
        std::uint8_t phys_page = static_cast<std::uint8_t>(p + (OLED_Y_OFFSET / 8));

        // Move column pointer to the start of the visible window
        setPageColumn(phys_page, static_cast<std::uint8_t>(OLED_X_OFFSET));

        // Start of this page in m_screen
        const std::uint8_t* row_ptr = m_screen.data() + (p * OLED_WIDTH);

        // Send the 72 bytes for this horizontal strip
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

    // Visible columns are 0–127
    column &= 0x7F;

    // Hardware column pointer is shifted by +1 on this module
    std::uint8_t hw_column = static_cast<std::uint8_t>(column + 1);

    // Set page
    sendCmd(static_cast<Command>(0xB0 | page));

    // Set lower 4 bits of column
    sendCmd(static_cast<Command>(0x00 | (hw_column & 0x0F)));

    // Set upper 4 bits of column
    sendCmd(static_cast<Command>(0x10 | ((hw_column >> 4) & 0x0F)));
}

} // namespace ssd1306
} // namespace muc
