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

#include "renderer_logger.h"

#include <ranges>

auto mir::renderer::gl::collect_capabilities() -> Capabilities
{
    Capabilities capabilities{
        eglGetCurrentDisplay(),
        {},
        {},
        0,
        {},
    };

    if (capabilities.display != EGL_NO_DISPLAY)
    {
        for (auto const& [index, eglstring] : Capabilities::eglstrings | std::views::enumerate)
        {
            auto const egl_string = eglQueryString(capabilities.display, eglstring.id);
            capabilities.egl_strings[index] = egl_string ? egl_string : "";
        }
    }

    for (auto const& [index, glstring] : Capabilities::glstrings | std::views::enumerate)
    {
        // TICS !cppcoreguidelines-pro-type-reinterpret-cast: glGetString returns an ASCII string,
        // guaranteed not to have the high-bit set, so it is representationally-identical to signed char.
        auto const gl_string = reinterpret_cast<char const*>(glGetString(glstring.id));
        capabilities.gl_strings[index] = gl_string ? gl_string : "";
    }

    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &capabilities.max_texture_size);

    std::array const framebuffer_queries{
        GL_RED_BITS,
        GL_GREEN_BITS,
        GL_BLUE_BITS,
        GL_ALPHA_BITS,
        GL_DEPTH_BITS,
        GL_STENCIL_BITS,
    };
    for (auto index = 0u; index != framebuffer_queries.size(); ++index)
        glGetIntegerv(framebuffer_queries[index], &capabilities.framebuffer_bits[index]);

    return capabilities;
}

void mir::renderer::gl::Logger::maybe_log_capabilities()
{
    auto const capabilities = collect_capabilities();

    if (!reported_capabilities.insert(capabilities))
        return;


    if (capabilities.display != EGL_NO_DISPLAY)
    {
        for (auto const& [index, eglstring] : Capabilities::eglstrings | std::views::enumerate)
            mir::log_info({mir::logging::graphics()}, "{}: {}", eglstring.label, capabilities.egl_strings[index]);
    }

    for (auto const& [index, glstring] : Capabilities::glstrings | std::views::enumerate)
        mir::log_info({mir::logging::graphics()}, "{}: {}", glstring.label, capabilities.gl_strings[index]);

    mir::log_info({mir::logging::graphics()}, "GL max texture size = {}", capabilities.max_texture_size);
    mir::log_info(
        {mir::logging::graphics()},
        "GL framebuffer bits: RGBA={}{}{}{}, depth={}, stencil={}",
        capabilities.framebuffer_bits[0],
        capabilities.framebuffer_bits[1],
        capabilities.framebuffer_bits[2],
        capabilities.framebuffer_bits[3],
        capabilities.framebuffer_bits[4],
        capabilities.framebuffer_bits[5]);
}
