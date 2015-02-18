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

#include "gltext.h"
#include <GLES2/gl2.h>  // TODO: Support plain OpenGL too

using namespace mir::examples::gltext;

Image::Image()
    : buf(nullptr), width(0), stride(0), height(0), align(4),
      format(GL_ALPHA)
{
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &align);
}

Image::~Image()
{
    delete[] buf;
}

void Image::reserve(int w, int h, GLenum fmt)
{
    width = w;
    height = h;
    format = fmt;
    int const bpp = (format == GL_ALPHA || format == GL_LUMINANCE) ? 1
                  : (format == GL_LUMINANCE_ALPHA) ? 2
                  : 4;
    stride = (((width * bpp) + align - 1) / align) * align;
    delete[] buf;
    buf = new GLubyte[stride * height];
}

Renderer::~Renderer()
{
}

Cache::Cache(std::shared_ptr<Renderer> const& r)
    : renderer(r)
{
}

Cache::~Cache()
{
    clear();
}

void Cache::clear()
{
    for (auto& e : map)
        glDeleteTextures(1, &e.second.tex);
    map.clear();
}

void Cache::mark_all_unused()
{
    for (auto& e : map)
        e.second.used = false;
}

void Cache::drop_unused()
{
    for (auto e = map.begin(); e != map.end();)
    {
        if (!e->second.used)
        {
            glDeleteTextures(1, &e->second.tex);
            e = map.erase(e);
        }
        else
            e++;
    }
}

bool Cache::Entry::valid() const
{
    return width > 0 && height > 0;
}

Cache::Entry const& Cache::get(char const* str)
{
    Entry& entry = map[str];
    if (!entry.valid())
    {
        Image img;
        renderer->render(str, img);
        if (img.buf)
        {
            entry.width = img.width;
            entry.height = img.height;
            glGenTextures(1, &entry.tex);
            glBindTexture(GL_TEXTURE_2D, entry.tex);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                                           GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexImage2D(GL_TEXTURE_2D, 0, img.format,
                         img.width, img.height, 0, img.format,
                         GL_UNSIGNED_BYTE, img.buf);
            glGenerateMipmap(GL_TEXTURE_2D); // Antialiasing shrinkage please
        }
    }
    entry.used = true;
    return entry;
}
