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

using namespace mir::typo;

Renderer::Image::Image()
    : buf(nullptr), width_(0), stride_(0), height_(0), align_(4),
      format_(alpha8)
{
}

Renderer::Image::~Image()
{
    delete[] buf;
}

void Renderer::Image::reserve(int w, int h, Format f)
{
    width_ = w;
    height_ = h;
    format_ = f;
    int const bpp = 1;  // format is always alpha8
    stride_ = (((width_ * bpp) + align_ - 1) / align_) * align_;
    delete[] buf;
    auto size = stride_ * height_;
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
