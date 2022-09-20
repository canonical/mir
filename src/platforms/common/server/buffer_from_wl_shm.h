/*
 * Copyright © 2019 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_GRAPHICS_GL_WAYLAND_SHM_PROVIDER_H_
#define MIR_GRAPHICS_GL_WAYLAND_SHM_PROVIDER_H_

#include <memory>
#include <functional>

struct wl_resource;

namespace mir
{
class Executor;

namespace graphics
{
class Buffer;

namespace common
{
class EGLContextExecutor;
}

namespace wayland
{
/**
 * Perform initialisation of SHM handling
 *
 * The returned handle must be kept live for as long as any buffer
 * returned by buffer_from_wl_shm is live.
 */
auto init_shm_handling() -> std::shared_ptr<void>;

/**
 * Get a mir::graphics::Buffer with the content of the shm buffer.
 *
 * The returned buffer will support the mg::gl::Texture and
 * mir::renderer::sw::PixelSource interfaces.
 *
 * \note This must be called on the Wayland thread, with a current GL context
 *
 * \param buffer        [in]    The Wayland SHM buffer to import
 * \param executor      [in]    An Executor that will defer work to the Wayland event loop
 * \param egl_delegate  [in]    An EGL-context-thread delegator
 * \param on_consumed   [in]    Closure to call when the compositor has consumed this buffer
 * \return                      An mg::Buffer supporting being rendered from in GL and read by the CPU.
 */
auto buffer_from_wl_shm(
    wl_resource* buffer,
    std::shared_ptr<Executor> executor,
    std::shared_ptr<common::EGLContextExecutor> egl_delegate,
    std::function<void()>&& on_consumed) -> std::shared_ptr<Buffer>;
}
}
}

#endif  // MIR_GRAPHICS_GL_WAYLAND_SHM_PROVIDER_H_
