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

#include "gl_text_cache.h"
#include <GLES2/gl2.h>  // TODO: Support plain OpenGL too

using mir::examples::GLTextCache;

GLTextCache::Image::Image()
    : buf(nullptr), width(0), stride(0), height(0), bpp(1), align(4)
{
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &align);
}

GLTextCache::Image::~Image()
{
    delete[] buf;
}

void GLTextCache::Image::reserve(int w, int h)
{
    width = w;
    height = h;
    stride = (((width * bpp) + align - 1) / align) * align;
    delete[] buf;
    buf = new GLubyte[stride * height];
}

GLTextCache::~GLTextCache()
{
    clear();
}

void GLTextCache::clear()
{
    for (auto& e : map)
        glDeleteTextures(1, &e.second.tex);

    map.clear();
}

bool GLTextCache::Entry::valid() const
{
    return width > 0 && height > 0;
}

GLTextCache::Entry const& GLTextCache::insert(char const* str,
                                              Render const render)
{
    Entry& entry = map[str];
    if (!entry.valid())
    {
        Image img;
        render(str, img);
        if (img.buf)
        {
            entry.width = img.width;
            entry.height = img.height;
            glGenTextures(1, &entry.tex);
            glBindTexture(GL_TEXTURE_2D, entry.tex);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA,
                         img.width, img.height, 0, GL_LUMINANCE_ALPHA,
                         GL_UNSIGNED_BYTE, img.buf);
        }
    }
    return entry;
}

