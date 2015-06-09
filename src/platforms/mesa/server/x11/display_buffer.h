/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
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
 * Authored by: Cemil Azizoglu <cemil.azizoglu@canonical.com>
 *
 */

#ifndef MIR_GRAPHICS_X_DISPLAY_BUFFER_H_
#define MIR_GRAPHICS_X_DISPLAY_BUFFER_H_

#include "mir/graphics/display_buffer.h"
#include "gl_context.h"

#include <EGL/egl.h>

namespace mir
{
namespace graphics
{
namespace X
{

class DisplayBuffer : public graphics::DisplayBuffer
{
public:
    DisplayBuffer(geometry::Size const sz, EGLDisplay const d, EGLSurface const s, EGLContext const c);

    geometry::Rectangle view_area() const override;
    void make_current() override;
    void release_current() override;
    void gl_swap_buffers() override;
    bool post_renderables_if_optimizable(RenderableList const& renderlist) override;

    MirOrientation orientation() const override;

private:
    geometry::Size const size;
    EGLDisplay const egl_dpy;
    EGLSurface const egl_surf;
    EGLContext const egl_ctx;
};

}
}
}

#endif /* MIR_GRAPHICS_X_DISPLAY_BUFFER_H_ */
