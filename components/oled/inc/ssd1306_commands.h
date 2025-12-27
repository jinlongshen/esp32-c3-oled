#ifndef MUC_OLED_SSD1306_COMMANDS_H
#define MUC_OLED_SSD1306_COMMANDS_H

#include <cstdint>

namespace muc::ssd1306
{

enum Command : std::uint8_t
{
    DisplayOff = 0xAE,
    DisplayOn = 0xAF,

    SetClockDiv = 0xD5,
    SetMultiplex = 0xA8,
    SetDisplayOffset = 0xD3,
    SetStartLine = 0x40,

    ChargePump = 0x8D,
    ChargePumpOn = 0x14,

    MemoryMode = 0x20,
    MemoryModeHorizontal = 0x00,

    SegmentRemap = 0xA1,
    ComScanDec = 0xC8,

    SetComPins = 0xDA,
    SetContrast = 0x81,
    SetPrecharge = 0xD9,
    SetVcomDetect = 0xDB,

    ResumeRAM = 0xA4,
    NormalDisplay = 0xA6,
};

} // namespace muc::ssd1306

#endif // MUC_OLED_SSD1306_COMMANDS_H
