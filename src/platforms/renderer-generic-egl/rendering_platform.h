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

#include "mir/graphics/platform.h"

#include <EGL/egl.h>

namespace mir
{

namespace graphics::egl::generic
{

class RenderingPlatform : public graphics::RenderingPlatform
{
public:
    explicit RenderingPlatform(std::vector<std::shared_ptr<DisplayInterfaceProvider>> const& displays);

    auto create_buffer_allocator(
        graphics::Display const& output) -> UniqueModulePtr<graphics::GraphicBufferAllocator> override;

protected:
    auto maybe_create_interface(
        RendererInterfaceBase::Tag const& type_tag) -> std::shared_ptr<RendererInterfaceBase> override;

private:
    EGLDisplay const dpy;
    EGLContext const ctx;
};

}
}
#endif /* MIR_GRAPHICS_RENDERING_EGL_GENERIC_H_ */
