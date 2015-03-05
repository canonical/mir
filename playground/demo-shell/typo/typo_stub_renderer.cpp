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

#include "typo_stub_renderer.h"
#include <cstring>

using namespace mir::examples::typo;

void StubRenderer::render(char const* str, Image& img)
{
    int const char_width = 8;
    int const char_height = 16;
    int const char_space = 2;
    int const tex_height = 20;
    int const len = strlen(str);
    int const top = (tex_height - char_height) / 2;

    img.reserve(len*(char_width+char_space) - char_space, tex_height,
                Image::alpha8);

    char const* s = str;
    for (int n = 0; unicode_from_utf8(&s); ++n)
    {
        unsigned char* row = img.buf + top*img.stride +
                             n*(char_width+char_space);
        for (int y = 0; y < char_height; ++y)
        {
            memset(row, 255, char_width);
            row += img.stride;
        }
    }
}
