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

#ifndef MIR_PLATFORMS_EGLSTREAM_KMS_PLATFORM_H_
#define MIR_PLATFORMS_EGLSTREAM_KMS_PLATFORM_H_

#include "mir/graphics/platform.h"
#include "mir/options/option.h"
#include "mir/graphics/graphic_buffer_allocator.h"
#include "mir/graphics/display.h"
#include "mir/fd.h"
#include "mir/renderer/gl/context_source.h"

#include <EGL/egl.h>
#include <EGL/eglext.h>

/* XXX khronos eglext.h does not yet have EGL_DRM_MASTER_FD_EXT */
#if !defined(EGL_DRM_MASTER_FD_EXT)
#define EGL_DRM_MASTER_FD_EXT                   (0x333C)
#endif

namespace mir
{
class Device;

namespace graphics
{
namespace eglstream
{

class RenderingPlatform : public graphics::RenderingPlatform
{
public:
    RenderingPlatform(EGLDisplay dpy);
    ~RenderingPlatform() override;

    UniqueModulePtr<GraphicBufferAllocator>
        create_buffer_allocator(Display const& output) override;

protected:
    auto maybe_create_interface(
        RendererInterfaceBase::Tag const& type_tag) -> std::shared_ptr<RendererInterfaceBase> override;
private:
    EGLDisplay const dpy;
    std::unique_ptr<renderer::gl::Context> const ctx;
};
}
}
}

#endif // MIR_PLATFORMS_EGLSTREAM_KMS_PLATFORM_H_
