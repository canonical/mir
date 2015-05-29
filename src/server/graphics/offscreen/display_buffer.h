/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_GRAPHICS_OFFSCREEN_DISPLAY_BUFFER_H_
#define MIR_GRAPHICS_OFFSCREEN_DISPLAY_BUFFER_H_

#include "mir/graphics/surfaceless_egl_context.h"

#include "mir/graphics/display_buffer.h"
#include "mir/geometry/size.h"
#include "mir/geometry/rectangle.h"

#include <EGL/egl.h>

namespace mir
{
namespace graphics
{
namespace offscreen
{

namespace detail
{

class GLFramebufferObject
{
public:
    GLFramebufferObject(geometry::Size const& size);
    ~GLFramebufferObject();
    void bind() const;
    void unbind() const;

private:
    geometry::Size const size;
    int old_fbo;
    int old_viewport[4];
    unsigned int color_renderbuffer;
    unsigned int depth_renderbuffer;
    unsigned int fbo;
};

}

class DisplayBuffer : public graphics::DisplayBuffer
{
public:
    DisplayBuffer(SurfacelessEGLContext egl_context,
                  geometry::Rectangle const& area);

    geometry::Rectangle view_area() const override;
    void make_current() override;
    void release_current() override;
    void gl_swap_buffers() override;

    MirOrientation orientation() const override;

    bool post_renderables_if_optimizable(RenderableList const& renderlist) override; 

private:
    SurfacelessEGLContext const egl_context;
    detail::GLFramebufferObject const fbo;
    geometry::Rectangle const area;
};

}
}
}

#endif /* MIR_GRAPHICS_OFFSCREEN_DISPLAY_BUFFER_H_ */
