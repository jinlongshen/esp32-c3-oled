#include "FontRenderer.h"

namespace muc::fonts
{

FontRenderer::FontRenderer() noexcept
: library{}
, face{}
{
    // blank
}

std::uint32_t decode_utf8(const char*& p) noexcept
{
    // Fetch the first byte and advance the pointer
    std::uint8_t c = static_cast<std::uint8_t>(*p++);

    // --- 1-BYTE SEQUENCE (Standard ASCII) ---
    // Range: U+0000 to U+007F
    // Bit pattern: 0xxxxxxx
    if (c < 0x80)
    {
        return c;
    }

    // --- 2-BYTE SEQUENCE (e.g., £, ©, or German Umlauts like 'ü') ---
    // Range: U+0080 to U+07FF
    // Bit pattern: 110xxxxx 10xxxxxx
    // (c & 0xE0) == 0xC0 checks if the first 3 bits are '110'
    if ((c & 0xE0) == 0xC0)
    {
        // (c & 0x1F): Keeps the 5 data bits from the first byte
        // << 6: Shifts them to the high position
        // (*p++ & 0x3F): Takes 6 data bits from the next byte (ignores the '10' prefix)
        return ((c & 0x1F) << 6) | (static_cast<std::uint8_t>(*p++) & 0x3F);
    }

    // --- 3-BYTE SEQUENCE (e.g., Euro sign '€' or Basic Multilingual Plane) ---
    // Range: U+0800 to U+FFFF
    // Bit pattern: 1110xxxx 10xxxxxx 10xxxxxx
    // (c & 0xF0) == 0xE0 checks if the first 4 bits are '1110'
    if ((c & 0xF0) == 0xE0)
    {
        std::uint8_t c2 = static_cast<std::uint8_t>(*p++);
        std::uint8_t c3 = static_cast<std::uint8_t>(*p++);

        // (c & 0x0F): Keeps 4 data bits from 1st byte (shfited 12)
        // (c2 & 0x3F): Keeps 6 data bits from 2nd byte (shifted 6)
        // (c3 & 0x3F): Keeps 6 data bits from 3rd byte
        return ((c & 0x0F) << 12) | ((c2 & 0x3F) << 6) | (c3 & 0x3F);
    }

    // Fallback for 4-byte sequences (Emoji/Ancient scripts) or invalid UTF-8
    return '?';
}

bool FontRenderer::init(const std::uint8_t* data, std::size_t size, int pixel_size) noexcept
{
    if (FT_Init_FreeType(&library))
    {
        return false;
    }
    if (FT_New_Memory_Face(library, data, static_cast<FT_Long>(size), 0, &face))
    {
        return false;
    }
    FT_Set_Pixel_Sizes(face, 0, pixel_size);
    return true;
}

FontRenderer::~FontRenderer() noexcept
{
    if (face)
    {
        FT_Done_Face(face);
    }
    if (library)
    {
        FT_Done_FreeType(library);
    }
}

} // namespace muc::fonts