#ifndef COMPONENTS_OLED_INC_DISPLAY_GEOMETRY_H
#define COMPONENTS_OLED_INC_DISPLAY_GEOMETRY_H

#include <cstdint>

namespace muc::ssd1306
{

struct DisplayGeometry
{
    int width;    // visible width in pixels
    int height;   // visible height in pixels
    int x_offset; // hardware column offset of visible area
    int y_offset; // hardware row/page offset (multiple of 8)

    int ram_width;  // SSD1306 RAM width (usually 128)
    int ram_height; // SSD1306 RAM height (usually 64)
    int ram_pages;  // ram_height / 8
};

// Default geometry for your 72×40 glass on a 128×64 SSD1306
constexpr DisplayGeometry kDefaultGeometry{.width = 72,
                                           .height = 40,
                                           .x_offset = 28,
                                           .y_offset = 24,
                                           .ram_width = 128,
                                           .ram_height = 64,
                                           .ram_pages = 64 / 8};

// Visible framebuffer bytes (LVGL draws only this window)
constexpr std::size_t kVisibleFramebufferBytes = static_cast<std::size_t>(kDefaultGeometry.width) *
                                                 static_cast<std::size_t>(kDefaultGeometry.height) /
                                                 8;

// LVGL I1 buffer = 8‑byte palette + framebuffer
constexpr std::size_t kVisibleLvglBufferBytes = 8 + kVisibleFramebufferBytes;

// Full SSD1306 RAM buffer (chip-level)
constexpr std::size_t kRamBufferBytes = static_cast<std::size_t>(kDefaultGeometry.ram_width) *
                                        static_cast<std::size_t>(kDefaultGeometry.ram_height) / 8;

// Page count
constexpr std::size_t kPageCount = static_cast<std::size_t>(kDefaultGeometry.ram_pages);

} // namespace muc::ssd1306

#endif // COMPONENTS_OLED_INC_DISPLAY_GEOMETRY_H
