#include "Text.hpp"

#include <locale>
#include <codecvt>

#include <coreinit/memory.h>

#include <ft2build.h>
#include FT_FREETYPE_H

static FT_Library ft_lib = nullptr;
static FT_Face ft_face = nullptr;

void Text::InitializeFont()
{
    // Initialize freetype
    FT_Init_FreeType(&ft_lib);

    // Load the system font
    void *font = nullptr;
    uint32_t size = 0;
    OSGetSharedData(OS_SHAREDDATATYPE_FONT_STANDARD, 0, &font, &size);

    // Create face
    if (font && size) {
        FT_New_Memory_Face(ft_lib, (FT_Byte*) font, size, 0, &ft_face);
    }
}

void Text::DeinitializeFont()
{
    // Finish face and lib
    FT_Done_Face(ft_face);
    FT_Done_FreeType(ft_lib);
}

Text::Text(std::string text, uint32_t textSize, glm::vec2 pos, glm::vec2 scale, float angle, glm::vec4 color) :
    Sprite(pos, glm::vec2(), angle, color),
    texture(nullptr)
{
    this->textSize = textSize;
    SetText(text);
}

Text::~Text()
{
    texture->Delete();
}

void Text::SetText(std::string text)
{
    // Convert the multi-byte string to a wstring
    static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    std::wstring wtext = converter.from_bytes(text);

    // Only redraw if the text actually changed
    if (this->text != wtext) {
        this->text = wtext;

        RedrawTexture();
    }
}

void Text::SetTextSize(uint32_t textSize)
{
    // Only redraw if the textSize actually changed
    if (this->textSize != textSize) {
        this->textSize = textSize;

        RedrawTexture();
    }
}

void Text::RedrawTexture()
{
    // Set the wanted text size
    FT_Set_Pixel_Sizes(ft_face, 0, textSize);

    // Calculate the necessary size for the texture
    const uint32_t face_height = ft_face->size->metrics.height >> 6;

    uint32_t width = 0;
    bounds = glm::uvec2(0, face_height);
    FT_GlyphSlot slot = ft_face->glyph;
    for (const wchar_t charcode : text) {
        if (charcode == '\n') {
            if (width > bounds.x) {
                bounds.x = width;
                width = 0;
            }
            bounds.y += face_height;
            continue;
        }

        FT_Load_Glyph(ft_face, FT_Get_Char_Index(ft_face, charcode), FT_LOAD_BITMAP_METRICS_ONLY);

        width += slot->advance.x >> 6;
    }

    if (width > bounds.x) {
        bounds.x = width;
    }

    // Add some extra height for the bottom bearing
    bounds.y += (ft_face->bbox.yMax - ft_face->bbox.yMin) >> 6;

    if (texture) {
        if (texture->GetSize() != bounds) {
            // Re-create the already existing texture if it doesn't match the wanted size
            texture->Delete();
            texture = Gfx::NewTexture(bounds);
        } else {
            // Clear the already existing texture
            void* data = texture->Lock();
            memset(data, 0, texture->GetSize().y * texture->GetPitch() * 4);
            texture->Unlock();
        }
    } else {
        // Allocate the texture
        texture = Gfx::NewTexture(bounds);
    }

    if (!texture) {
        return;
    }

    // Get pitch and lock the texture
    uint32_t pitch = texture->GetPitch();
    uint8_t* pixels = (uint8_t*) texture->Lock();

    // Render the glyphs into the texture
    FT_Vector pen = { 0, 0 };
    for (const wchar_t charcode : text) {
        if (charcode == '\n') {
            pen.x = 0;
            pen.y += ft_face->size->metrics.height >> 6;
            continue;
        }

        FT_Load_Glyph(ft_face, FT_Get_Char_Index(ft_face, charcode), FT_LOAD_DEFAULT);
        FT_Render_Glyph(slot, FT_RENDER_MODE_NORMAL);

        const FT_Int x = pen.x + slot->bitmap_left;
        const FT_Int y = (pen.y - slot->bitmap_top) + face_height;
        const FT_Int x_max = x + slot->bitmap.width;
        const FT_Int y_max = y + slot->bitmap.rows;

        for (FT_Int i = x, p = 0; i < x_max; ++i, ++p) {
            for (FT_Int j = y, q = 0; j < y_max; ++j, ++q) {
                if (i < 0 || j < 0 || i >= (FT_Int) bounds.x || j >= (FT_Int) bounds.y) {
                    continue;
                }

                uint32_t offset = (j * pitch + i) * 4;
                pixels[offset    ] = 255;
                pixels[offset + 1] = 255;
                pixels[offset + 2] = 255;
                pixels[offset + 3] = slot->bitmap.buffer[q * slot->bitmap.pitch + p];
            }
        }

        pen.x += slot->advance.x >> 6;
    }

    // Unlock the finished texture
    texture->Unlock();

    // Set the texture and scale of the underlying sprite
    SetTexture(texture, true);
    SetScale(scale);
}
