/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Daniel van Vugt <daniel.van.vugt@canonical.com>
 */

#include "typo_freetype_renderer.h"
#include <stdexcept>
#include <cstring>

using namespace mir::examples::typo;

FreetypeRenderer::FreetypeRenderer()
    : lib(nullptr), face(nullptr), preferred_height(16)
{
    if (FT_Init_FreeType(&lib))
        throw std::runtime_error("FreeType init failed");
}

FreetypeRenderer::~FreetypeRenderer()
{
    if (face) FT_Done_Face(face);
    if (lib) FT_Done_FreeType(lib);
}

bool FreetypeRenderer::load(char const* font_path, int pref_height)
{
    preferred_height = pref_height;

    if (face)
    {
        FT_Done_Face(face);
        face = nullptr;
    }
    if (FT_New_Face(lib, font_path, 0, &face))
        return false;

    FT_Set_Pixel_Sizes(face, 0, preferred_height);

    return true;
}

void FreetypeRenderer::render(char const* str, Image& img)
{
    int minx = 0, maxx = 0, miny = 0, maxy = 0;
    int penx = 0, peny = 0;
    FT_GlyphSlot slot = face->glyph;

    char const* s = str;
    while (unsigned long u = unicode_from_utf8(&s))
    {
        auto glyph = FT_Get_Char_Index(face, u);
        FT_Load_Glyph(face, glyph, FT_LOAD_DEFAULT);
        FT_Render_Glyph(slot, FT_RENDER_MODE_NORMAL);

        int left = penx + slot->bitmap_left;
        if (left < minx) minx = left;

        int right = left + slot->bitmap.width;
        if (right > maxx) maxx = right;

        int top = peny - slot->bitmap_top;
        if (top < miny) miny = top;

        int bottom = top + slot->bitmap.rows;
        if (bottom > maxy) maxy = bottom;

        penx += slot->advance.x >> 6;
        peny += slot->advance.y >> 6;
    }

    int const padding = preferred_height / 8;  // Allow mipmapping to smear
    int width = maxx - minx + 1 + 2*padding;
    int height = maxy - miny + 1;
    if (height < preferred_height)  // e.g. str has no descenders, but make
        height = preferred_height;  //      room so we get a consistent height
    height += 2*padding;
    penx = -minx + padding;
    peny = -miny + padding;

    img.reserve(width, height, Image::alpha8);

    s = str;
    while (unsigned long u = unicode_from_utf8(&s))
    {
        auto glyph = FT_Get_Char_Index(face, u);
        FT_Load_Glyph(face, glyph, FT_LOAD_DEFAULT);
        FT_Render_Glyph(slot, FT_RENDER_MODE_NORMAL);

        auto& bitmap = slot->bitmap;
        int x = penx + slot->bitmap_left;
        int y = peny - slot->bitmap_top;
        int right = x + bitmap.width;
        int bottom = y + bitmap.rows;

        if (x >= 0 && right <= img.width && y >= 0 && bottom <= img.height)
        {
            unsigned char* src = bitmap.buffer;
            unsigned char* dest = img.buf + y*img.stride + x;

            int ylimit = y + bitmap.rows;
            for (; y < ylimit; ++y)
            {
                memcpy(dest, src, bitmap.width);
                src += bitmap.pitch;
                dest += img.stride;
            }
        }

        penx += slot->advance.x >> 6;
        peny += slot->advance.y >> 6;
    }
}
