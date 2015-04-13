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

#include "display_buffer.h"
#include "../debug.h"
#include <boost/throw_exception.hpp>

namespace mg=mir::graphics;
namespace mgx=mg::X;
namespace geom=mir::geometry;

mgx::DisplayBuffer::DisplayBuffer(geom::Size const sz,
		                          EGLDisplay const d,
                                  EGLSurface const s,
                                  EGLContext const c)
                                  : size{sz}, egl_dpy{d}, egl_surf{s}, egl_ctx{c}
{
	CALLED
}

geom::Rectangle mgx::DisplayBuffer::view_area() const
{
	CALLED
    int width = size.width.as_int();
    int height = size.height.as_int();

    return {{0,0}, {width,height}};
}

void mgx::DisplayBuffer::make_current()
{
	CALLED
    if (!eglMakeCurrent(egl_dpy, egl_surf, egl_surf, egl_ctx))
        BOOST_THROW_EXCEPTION(std::logic_error("Cannot make current"));
}

void mgx::DisplayBuffer::release_current()
{
	CALLED
    if (!eglMakeCurrent(egl_dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT))
        BOOST_THROW_EXCEPTION(std::logic_error("Cannot make uncurrent"));
}

bool mgx::DisplayBuffer::post_renderables_if_optimizable(RenderableList const& /*renderlist*/)
{
	CALLED
    return false;
}

void mgx::DisplayBuffer::gl_swap_buffers()
{
	CALLED
	if (!eglSwapBuffers(egl_dpy, egl_surf))
        BOOST_THROW_EXCEPTION(std::logic_error("Cannot swap"));
}

MirOrientation mgx::DisplayBuffer::orientation() const
{
	CALLED
    return mir_orientation_normal;
}

bool mgx::DisplayBuffer::uses_alpha() const
{
	CALLED
    return false;
}
