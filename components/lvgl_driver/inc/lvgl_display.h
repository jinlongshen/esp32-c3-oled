#pragma once

#include <cstdint>
#include "lvgl.h"
#include "ssd1306.h"

namespace muc::lvgl_driver
{

// Initialize LVGL display using compile‑time geometry and internal buffers.
void init_display(lv_display_t& disp,
                  lv_draw_buf_t& draw_buf_unused,
                  muc::ssd1306::Oled& oled,
                  std::uint8_t* buf_memory_unused,
                  std::size_t buf_size_unused);

} // namespace muc::lvgl_driver
