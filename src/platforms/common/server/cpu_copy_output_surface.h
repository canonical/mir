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

#include <EGL/egl.h>
#include <GLES2/gl2.h>

#include <memory>

#include "mir/graphics/drm_formats.h"
#include "mir/geometry/size.h"
#include "mir/graphics/platform.h"
#include "mir/renderer/gl/gl_surface.h"

namespace mir::graphics
{

namespace common
{
class CPUCopyOutputSurface : public gl::OutputSurface
{
public:
    CPUCopyOutputSurface(
        EGLDisplay dpy,
        EGLContext share_ctx,
        CPUAddressableDisplayAllocator& allocator);

    ~CPUCopyOutputSurface() override;

    void bind() override;

    void make_current() override;

    void release_current() override;

    auto commit() -> std::unique_ptr<Framebuffer> override;

    auto size() const -> geometry::Size override;

    auto layout() const -> Layout override;

private:
    class Impl;
    std::unique_ptr<Impl> const impl;
};
}
}
