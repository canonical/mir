/* Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_RENDERER_GL_RENDERER_LOGGER_H_
#define MIR_RENDERER_GL_RENDERER_LOGGER_H_

#include <mir/recent_items.h>
#include <mir/synchronised.h>
#include <mir/log.h>

#include <GL/gl.h>
#include <EGL/egl.h>


namespace mir
{
namespace renderer
{
namespace gl
{

struct Capabilities
{
    struct
    {
        GLint id;
        std::string_view label;
    } static constexpr eglstrings[] = {
        {EGL_VENDOR, "EGL vendor"},
        {EGL_VERSION, "EGL version"},
        {EGL_CLIENT_APIS, "EGL client APIs"},
        {EGL_EXTENSIONS, "EGL extensions"},
    };

    struct
    {
        GLenum id;
        std::string_view label;
    } static constexpr glstrings[] = {
        {GL_VENDOR, "GL vendor"},
        {GL_RENDERER, "GL renderer"},
        {GL_VERSION, "GL version"},
        {GL_SHADING_LANGUAGE_VERSION, "GLSL version"},
        {GL_EXTENSIONS, "GL extensions"},
    };

    EGLDisplay display;
    std::array<std::string, 4> egl_strings;
    std::array<std::string, 5> gl_strings;
    GLint max_texture_size;
    std::array<GLint, 6> framebuffer_bits;

    auto operator==(Capabilities const&) const -> bool = default;
};

auto collect_capabilities() -> Capabilities;

class Logger
{
public:
    Logger() = default;

    // Logs GL and EGL capabilities only if the current capabilities have not been logged before.
    void maybe_log_capabilities();

private:
    template<typename T, std::size_t Capacity>
    class BoundedSet
    {
    public:
        auto insert(T const& value) -> bool
        {
            auto values = items.lock();

            if (values->contains(value))
                return false;

            values->add(value);
            return true;
        }

    private:
        mir::Synchronised<mir::RecentItems<T, Capacity>> items;
    };

    BoundedSet<Capabilities, 32> reported_capabilities;
};
}
}
}

#endif
