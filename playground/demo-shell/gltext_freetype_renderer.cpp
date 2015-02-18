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

#include "gltext_freetype_renderer.h"
#include <cstring>

using namespace mir::examples::gltext;

FreetypeRenderer::FreetypeRenderer()
    : lib(nullptr), face(nullptr)
{
    if (FT_Init_FreeType(&lib))
        throw std::runtime_error("FreeType init failed");

    if (FT_New_Face(lib, "/usr/share/fonts/truetype/freefont/FreeSans.ttf", 0,
                    &face))
        throw std::runtime_error("FreeType couldn't get face");

    FT_Set_Char_Size(
          face,    /* handle to face object           */
          0,       /* char_width in 1/64th of points  */
          16*64,   /* char_height in 1/64th of points */
          300,     /* horizontal device resolution    */
          300);    /* vertical device resolution      */
}

FreetypeRenderer::~FreetypeRenderer()
{
    FT_Done_Face(face);
    FT_Done_FreeType(lib);
}

void FreetypeRenderer::render(char const* str, Image& img)
{
    int minx = 0, maxx = 0, miny = 0, maxy = 0;
    int penx = 0, peny = 0;
    FT_GlyphSlot slot = face->glyph;
    int len = strlen(str);

    for (int i = 0; i < len; ++i)
    {
        FT_ULong unicode = str[i];  // TODO: Add UTF-8 decoding
        auto glyph = FT_Get_Char_Index(face, unicode);
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

    fprintf(stderr, "(%d,%d) -> (%d,%d)\n", minx,miny, maxx,maxy);

    int width = maxx - minx + 1;
    int height = maxy - miny + 1;
    penx = -minx;
    peny = -miny;

    img.reserve(width, height, GL_ALPHA);
    fprintf(stderr, "allocate %dx%d\n", width, height);
    memset(img.buf, 0, img.stride * img.height);

    for (int i = 0; i < len; ++i)
    {
        FT_ULong unicode = str[i];  // TODO: Add UTF-8 decoding
        auto glyph = FT_Get_Char_Index(face, unicode);
        FT_Load_Glyph(face, glyph, FT_LOAD_DEFAULT);
        FT_Render_Glyph(slot, FT_RENDER_MODE_NORMAL);

        auto& bitmap = slot->bitmap;
        fprintf(stderr, "'%c': pixel mode %d, grays %hd\n",
                str[i], bitmap.pixel_mode, bitmap.num_grays);

        int x = penx + slot->bitmap_left;
        int y = peny - slot->bitmap_top;

        if (x >= 0 && x+bitmap.width <= img.width &&
            y >= 0 && y+bitmap.rows <= img.height)
        {
            unsigned char* src = bitmap.buffer;
            GLubyte* dest = img.buf + y*img.stride + x;

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
