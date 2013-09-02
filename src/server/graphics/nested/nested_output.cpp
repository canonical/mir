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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "nested_output.h"

#include "mir_toolkit/mir_client_library.h"

#include <boost/throw_exception.hpp>
#include <stdexcept>

namespace mgn = mir::graphics::nested;
namespace geom = mir::geometry;

mgn::detail::MirSurfaceHandle::MirSurfaceHandle(MirSurface* mir_surface) :
    mir_surface(mir_surface)
{
}

mgn::detail::MirSurfaceHandle::~MirSurfaceHandle() noexcept
{
    mir_surface_release_sync(mir_surface);
}

EGLint const mgn::detail::egl_attribs[] = {
    EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
    EGL_RED_SIZE, 8,
    EGL_GREEN_SIZE, 8,
    EGL_BLUE_SIZE, 8,
    EGL_ALPHA_SIZE, 8,
    EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
    EGL_NONE
};

EGLint const mgn::detail::egl_context_attribs[] = {
    EGL_CONTEXT_CLIENT_VERSION, 2,
    EGL_NONE
};

mgn::detail::NestedOutput::NestedOutput(
    EGLDisplayHandle const& egl_display,
    MirSurface* mir_surface,
    geometry::Rectangle const& area) :
    egl_display(egl_display),
    mir_surface{mir_surface},
    egl_config{egl_display.choose_config(egl_attribs)},
    egl_context{egl_display, eglCreateContext(egl_display, egl_config, EGL_NO_CONTEXT, egl_context_attribs)},
    area{area.top_left, area.size},
    egl_surface{EGL_NO_SURFACE}
{
}

geom::Rectangle mgn::detail::NestedOutput::view_area() const
{
    return area;
}

void mgn::detail::NestedOutput::make_current()
{
    egl_surface = egl_display.egl_surface(egl_config, mir_surface);

    if (eglMakeCurrent(egl_display, egl_surface, egl_surface, egl_context) != EGL_TRUE)
        BOOST_THROW_EXCEPTION(std::runtime_error("Nested Mir Display Error: Failed to update EGL surface.\n"));
}

void mgn::detail::NestedOutput::release_current()
{
    eglMakeCurrent(egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroySurface(egl_display, egl_surface);
    egl_surface = EGL_NO_SURFACE;
}

void mgn::detail::NestedOutput::post_update()
{
    mir_surface_swap_buffers_sync(mir_surface);
}

bool mgn::detail::NestedOutput::can_bypass() const
{
    // TODO we really should return "true" - but we need to support bypass properly then
    return false;
}


mgn::detail::NestedOutput::~NestedOutput() noexcept
{
    if (egl_surface != EGL_NO_SURFACE)
        eglDestroySurface(egl_display, egl_surface);
}


