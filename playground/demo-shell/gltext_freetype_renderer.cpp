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

#if 0
namespace {

void draw_fake(FT_Bitmap* bitmap, FT_Int x, FT_Int y)
{
  FT_Int  i, j, p, q;
  FT_Int  x_max = x + bitmap->width;
  FT_Int  y_max = y + bitmap->rows;


  for ( i = x, p = 0; i < x_max; i++, p++ )
  {
    for ( j = y, q = 0; j < y_max; j++, q++ )
    {
      if ( i < 0      || j < 0       ||
           i >= WIDTH || j >= HEIGHT )
        continue;

      image[j][i] |= bitmap->buffer[q * bitmap->width + p];
    }
  }
}

} // namespace
#endif

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

        int x = penx + slot->bitmap_left;
        if (x < minx) minx = x;
        x += slot->bitmap.width;
        if (x > maxx) maxx = x;

        int y = peny - slot->bitmap_top;
        if (y < miny) miny = y;
        y += slot->bitmap.rows;
        if (y > maxy) maxy = y;

        penx += slot->advance.x >> 6;
        peny += slot->advance.y >> 6; // probably won't change
    }

    fprintf(stderr, "(%d,%d) -> (%d,%d)\n", minx,miny, maxx,maxy);

    int width = maxx - minx + 1;
    int height = maxy - miny + 1;
    penx = -minx;
    peny = -miny;

    img.reserve(width, height, GL_ALPHA);
}
