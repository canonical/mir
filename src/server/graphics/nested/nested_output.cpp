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

#define MIR_INCLUDE_DEPRECATED_EVENT_HEADER

#include "nested_output.h"
#include "host_connection.h"
#include "mir/input/input_dispatcher.h"
#include "mir/graphics/pixel_format_utils.h"

#include <boost/throw_exception.hpp>
#include <stdexcept>

namespace mg = mir::graphics;
namespace mgn = mir::graphics::nested;
namespace geom = mir::geometry;

mgn::detail::NestedOutput::NestedOutput(
    EGLDisplayHandle const& egl_display,
    std::shared_ptr<HostSurface> const& host_surface,
    geometry::Rectangle const& area,
    std::shared_ptr<input::InputDispatcher> const& dispatcher,
    MirPixelFormat preferred_format) :
    uses_alpha_{mg::contains_alpha(preferred_format)},
    egl_display(egl_display),
    host_surface{host_surface},
    egl_config{egl_display.choose_windowed_es_config(preferred_format)},
    egl_context{egl_display, eglCreateContext(egl_display, egl_config, egl_display.egl_context(), nested_egl_context_attribs)},
    area{area.top_left, area.size},
    dispatcher{dispatcher},
    egl_surface{egl_display, host_surface->egl_native_window(), egl_config}
{
    MirEventDelegate ed = {event_thunk, this};
    host_surface->set_event_handler(&ed);
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

void mgn::detail::NestedOutput::gl_swap_buffers()
{
    eglSwapBuffers(egl_display, egl_surface);
}

void mgn::detail::NestedOutput::flip()
{
}

bool mgn::detail::NestedOutput::post_renderables_if_optimizable(RenderableList const&)
{
    return false;
}

MirOrientation mgn::detail::NestedOutput::orientation() const
{
    /*
     * Always normal orientation. The real rotation is handled by the
     * native display.
     */
    return mir_orientation_normal;
}

bool mgn::detail::NestedOutput::uses_alpha() const
{
    return uses_alpha_;
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
        dispatcher->dispatch(my_event);
    }
    else
    {
        dispatcher->dispatch(event);
    }
}
