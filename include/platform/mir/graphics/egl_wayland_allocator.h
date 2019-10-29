/*
 * Copyright © 2019 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by:
 *   Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#ifndef MIR_PLATFORM_EGL_WAYLAND_ALLOCATOR_H_
#define MIR_PLATFORM_EGL_WAYLAND_ALLOCATOR_H_

#include <memory>
#include <functional>
#include <EGL/egl.h>

struct wl_resource;
struct wl_display;

namespace mir
{
class Executor;

namespace renderer
{
namespace gl
{
class Context;
}
}

namespace graphics
{
class Buffer;
class EGLExtensions;

namespace wayland
{
void bind_display(EGLDisplay egl_dpy, wl_display* wl_dpy, EGLExtensions const& extensions);

auto buffer_from_resource(
    wl_resource* buffer,
    std::function<void()>&& on_consumed,
    std::function<void()>&& on_release,
    std::shared_ptr<renderer::gl::Context> ctx,
    EGLExtensions const& extensions,
    std::shared_ptr<Executor> wayland_executor) -> std::unique_ptr<Buffer>;

}
}
}


#endif  // MIR_PLATFORM_EGL_WAYLAND_ALLOCATOR_H_
