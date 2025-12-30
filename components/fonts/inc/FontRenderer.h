#ifndef COMPONENTS_FONTS_FONT_RENDERER_H
#define COMPONENTS_FONTS_FONT_RENDERER_H

#include <cstdint>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

namespace muc::fonts
{

// Decodes UTF-8 and advances the pointer
std::uint32_t decode_utf8(const char*& p);

class FontRenderer
{
  public:
    FontRenderer() noexcept;
    ~FontRenderer() noexcept;

    // Prevent copying to avoid double-freeing FT_Face
    FontRenderer(const FontRenderer&) = delete;
    FontRenderer& operator=(const FontRenderer&) = delete;

    bool init(const std::uint8_t* data, std::size_t size, int pixel_size) noexcept;

    FT_Library library;
    FT_Face face;
};

} // namespace muc::fonts

#endif // COMPONENTS_FONTS_FONT_RENDERER_H