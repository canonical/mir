/*
 * Copyright © Canonical Ltd.
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
 */

#ifndef MIR_GRAPHICS_RENDERING_EGL_GENERIC_H_
#define MIR_GRAPHICS_RENDERING_EGL_GENERIC_H_

#include "mir/graphics/linux_dmabuf.h"
#include "mir/graphics/platform.h"

#include <EGL/egl.h>

namespace mir
{
namespace renderer::gl
{
class Context;
}

namespace graphics::egl::generic
{

class RenderingPlatform : public graphics::RenderingPlatform
{
public:
    explicit RenderingPlatform(std::vector<std::shared_ptr<DisplayPlatform>> const& displays);

    ~RenderingPlatform();

    auto create_buffer_allocator(
        graphics::Display const& output) -> UniqueModulePtr<graphics::GraphicBufferAllocator> override;

protected:
    auto maybe_create_provider(
        RenderingProvider::Tag const& type_tag) -> std::shared_ptr<RenderingProvider> override;

private:
    explicit RenderingPlatform(std::tuple<EGLDisplay, bool> dpy);

    /* 
     * We (sometimes) need to clean up the EGLDisplay with eglTerminate,
     * but this should happen after cleanup of `ctx` and `dmabuf_provider`
     * (which both wrap resources on the EGLDisplay).
     *
     * Use a trivial handle class to handle cleanup on destruction.
     */
    class EGLDisplayHandle
    {
    public:
        EGLDisplayHandle(EGLDisplay dpy, bool owns_dpy);
        ~EGLDisplayHandle();

        operator EGLDisplay() const;

    private:
        EGLDisplay const dpy;
        bool const owns_dpy;
    };

    EGLDisplayHandle dpy;
    std::unique_ptr<renderer::gl::Context> const ctx;
    std::shared_ptr<DMABufEGLProvider> const dmabuf_provider;
    std::shared_ptr<common::EGLContextExecutor> const egl_delegate;
};

}
}
#endif /* MIR_GRAPHICS_RENDERING_EGL_GENERIC_H_ */
