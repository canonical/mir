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

#include "typo_glcache.h"
#include <GLES2/gl2.h>  // TODO: Support plain OpenGL too

using namespace mir::examples::typo;

GLCache::GLCache(std::shared_ptr<Renderer> const& r)
    : renderer(r)
{
}

GLCache::~GLCache()
{
    clear();
}

void GLCache::change_renderer(std::shared_ptr<Renderer> const& r)
{
    clear();
    renderer = r;
}

void GLCache::clear()
{
    for (auto& e : map)
        glDeleteTextures(1, &e.second.tex);
    map.clear();
}

void GLCache::mark_all_unused()
{
    for (auto& e : map)
        e.second.used = false;
}

void GLCache::drop_unused()
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

bool GLCache::Entry::valid() const
{
    return width > 0 && height > 0;
}

GLCache::Entry const& GLCache::get(char const* str)
{
    Entry& entry = map[str];
    if (!entry.valid())
    {
        Renderer::Image img;
        renderer->render(str, img);
        if (img.buf)
        {
            entry.width = img.width;
            entry.height = img.height;
            glGenTextures(1, &entry.tex);
            glBindTexture(GL_TEXTURE_2D, entry.tex);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                                           GL_LINEAR_MIPMAP_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glPixelStorei(GL_UNPACK_ALIGNMENT, img.align);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA,
                         img.width, img.height, 0, GL_ALPHA,
                         GL_UNSIGNED_BYTE, img.buf);
            glGenerateMipmap(GL_TEXTURE_2D); // Antialiasing shrinkage please
        }
    }
    entry.used = true;
    return entry;
}
