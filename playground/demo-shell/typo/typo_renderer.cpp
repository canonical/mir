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

#include "typo_renderer.h"
#include <cstring>

using namespace mir::examples::typo;

Renderer::Image::Image()
    : buf(nullptr), width(0), stride(0), height(0), align(4), format(alpha8)
{
}

Renderer::Image::~Image()
{
    delete[] buf;
}

void Renderer::Image::reserve(int w, int h, Format f)
{
    width = w;
    height = h;
    format = f;
    int const bpp = 1;  // format is always alpha8
    stride = (((width * bpp) + align - 1) / align) * align;
    delete[] buf;
    auto size = stride * height;
    buf = new unsigned char[size];
    memset(buf, 0, size);
}

Renderer::~Renderer()
{
}

unsigned long Renderer::unicode_from_utf8(char const** utf8)
{
    int char_len = 1; // TODO: Add support for non-ASCII UTF-8
    unsigned long unicode = **utf8;
    if (unicode)
        *utf8 += char_len;
    return unicode;
}
