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
#include "mir/input/event_filter.h"

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

mgn::detail::NestedOutput::NestedOutput(
    EGLDisplayHandle const& egl_display,
    MirSurface* mir_surface,
    geometry::Rectangle const& area,
    std::shared_ptr<input::EventFilter> const& event_handler,
    MirPixelFormat preferred_format) :
    egl_display(egl_display),
    mir_surface{mir_surface},
    egl_config{egl_display.choose_windowed_es_config(preferred_format)},
    egl_context{egl_display, eglCreateContext(egl_display, egl_config, egl_display.egl_context(), nested_egl_context_attribs)},
    area{area.top_left, area.size},
    event_handler{event_handler},
    egl_surface{egl_display, egl_display.native_window(egl_config, mir_surface), egl_config}
{
    MirEventDelegate ed = {event_thunk, this};
    mir_surface_set_event_handler(mir_surface, &ed);
}

geom::Rectangle mgn::detail::NestedOutput::view_area() const
{
    return area;
}

void mgn::detail::NestedOutput::make_current()
{
    if (eglMakeCurrent(egl_display, egl_surface, egl_surface, egl_context) != EGL_TRUE)
        BOOST_THROW_EXCEPTION(std::runtime_error("Nested Mir Display Error: Failed to update EGL surface.\n"));
}

void mgn::detail::NestedOutput::release_current()
{
    eglMakeCurrent(egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
}

void mgn::detail::NestedOutput::post_update()
{
    eglSwapBuffers(egl_display, egl_surface);
}

bool mgn::detail::NestedOutput::can_bypass() const
{
    // TODO we really should return "true" - but we need to support bypass properly then
    return false;
}

mgn::detail::NestedOutput::~NestedOutput() noexcept
{
}

void mgn::detail::NestedOutput::event_thunk(
    MirSurface* /*surface*/,
    MirEvent const* event,
    void* context)
try
{
    static_cast<mgn::detail::NestedOutput*>(context)->mir_event(*event);
}
catch (std::exception const&)
{
    // Just in case: do not allow exceptions to propagate.
}

void mgn::detail::NestedOutput::mir_event(MirEvent const& event)
{
    if (event.type == mir_event_type_motion)
    {
        auto my_event = event;
        my_event.motion.x_offset += area.top_left.x.as_float();
        my_event.motion.y_offset += area.top_left.y.as_float();
        event_handler->handle(my_event);
    }
    else
    {
        event_handler->handle(event);
    }
}
