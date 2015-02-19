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

#ifndef MIR_EXAMPLES_TYPO
#define MIR_EXAMPLES_TYPO

#include <GLES2/gl2.h>  // TODO: Support plain OpenGL too
#include <string>
#include <unordered_map>
#include <memory>

namespace mir { namespace examples { namespace typo {

struct Image
{
    Image();
    ~Image();
    void reserve(int w, int h, GLenum fmt);
    GLubyte *buf;
    int width, stride, height, align;
    GLenum format;  // glTexImage2D format
};

class Renderer
{
public:
    virtual ~Renderer();
    virtual void render(char const* str, Image& img) = 0;
protected:
    static unsigned long unicode_from_utf8(char const** utf8);
};

class Cache
{
public:
    struct Entry
    {
        bool valid() const;
        GLuint tex = 0;
        int width = 0, height = 0;
        bool used = false;
    };

    explicit Cache(std::shared_ptr<Renderer> const&);
    ~Cache();
    void change_renderer(std::shared_ptr<Renderer> const&);
    Entry const& get(char const* str);
    void clear();
    void mark_all_unused();
    void drop_unused();

private:
    typedef std::unordered_map<std::string, Entry> Map;
    Map map;
    std::shared_ptr<Renderer> renderer;
};

} } } // namespace mir::examples::typo

#endif // MIR_EXAMPLES_TYPO
