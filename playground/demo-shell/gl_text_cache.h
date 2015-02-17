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

#ifndef MIR_EXAMPLES_GL_TEXT_CACHE_
#define MIR_EXAMPLES_GL_TEXT_CACHE_

#include <GLES2/gl2.h>  // TODO: Support plain OpenGL too
#include <string>
#include <unordered_map>

namespace mir { namespace examples {

class GLTextCache
{
public:
    struct Entry
    {
        bool valid() const;
        GLuint tex = 0;
        int width = 0, height = 0;
        // Note: Image is not stored in the cache. It's all on-GPU as tex.
    };

    struct Image
    {
        Image();
        ~Image();
        void reserve(int w, int h);
        GLubyte *buf;
        int width, stride, height, bpp, align;
    };
    typedef void (*Render)(char const* str, Image& img);

    ~GLTextCache();
    Entry const& insert(char const* str, Render const render);
    void clear();

private:
    typedef std::unordered_map<std::string, Entry> Map;
    Map map;
};

} } // namespace mir::examples
#endif // MIR_EXAMPLES_GL_TEXT_CACHE_
