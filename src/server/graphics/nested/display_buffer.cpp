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

#include "display_buffer.h"

#include "host_connection.h"
#include "mir/input/input_dispatcher.h"
#include "mir/graphics/pixel_format_utils.h"
#include "mir/input/cursor_listener.h"
#include "mir/graphics/egl_error.h"
#include "mir/events/event_private.h"

#include <boost/throw_exception.hpp>
#include <stdexcept>

namespace mi = mir::input;
namespace mg = mir::graphics;
namespace mgn = mir::graphics::nested;
namespace geom = mir::geometry;

mgn::detail::DisplayBuffer::DisplayBuffer(
    EGLDisplayHandle const& egl_display,
    std::shared_ptr<HostSurface> const& host_surface,
    geometry::Rectangle const& area,
    std::shared_ptr<input::InputDispatcher> const& dispatcher,
    std::shared_ptr<mi::CursorListener> const& cursor_listener,
    MirPixelFormat preferred_format) :
    egl_display(egl_display),
    host_surface{host_surface},
    egl_config{egl_display.choose_windowed_es_config(preferred_format)},
    egl_context{egl_display, eglCreateContext(egl_display, egl_config, egl_display.egl_context(), nested_egl_context_attribs)},
    area{area.top_left, area.size},
    dispatcher{dispatcher},
    cursor_listener{cursor_listener},
    egl_surface{egl_display, host_surface->egl_native_window(), egl_config}
{
    host_surface->set_event_handler(event_thunk, this);
}

geom::Rectangle mgn::detail::DisplayBuffer::view_area() const
{
    return area;
}

void mgn::detail::DisplayBuffer::make_current()
{
    if (eglMakeCurrent(egl_display, egl_surface, egl_surface, egl_context) != EGL_TRUE)
        BOOST_THROW_EXCEPTION(mg::egl_error("Nested Mir Display Error: Failed to update EGL surface"));
}

void mgn::detail::DisplayBuffer::release_current()
{
    eglMakeCurrent(egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
}

void mgn::detail::DisplayBuffer::gl_swap_buffers()
{
    eglSwapBuffers(egl_display, egl_surface);
}

bool mgn::detail::DisplayBuffer::post_renderables_if_optimizable(RenderableList const&)
{
    return false;
}

MirOrientation mgn::detail::DisplayBuffer::orientation() const
{
    /*
     * Always normal orientation. The real rotation is handled by the
     * native display.
     */
    return mir_orientation_normal;
}

mgn::detail::DisplayBuffer::~DisplayBuffer() noexcept
{
}

void mgn::detail::DisplayBuffer::event_thunk(
    MirSurface* /*surface*/,
    MirEvent const* event,
    void* context)
try
{
    static_cast<mgn::detail::DisplayBuffer*>(context)->mir_event(*event);
}
catch (std::exception const&)
{
    // Just in case: do not allow exceptions to propagate.
}

void mgn::detail::DisplayBuffer::mir_event(MirEvent const& event)
{
    if (mir_event_get_type(&event) != mir_event_type_input)
        return;

    if (event.type == mir_event_type_motion)
    {
        auto my_event = event;
        auto iev = mir_event_get_input_event(&my_event);

        if (mir_input_event_get_type(iev) == mir_input_event_type_pointer)
        {
            auto& motion = my_event.motion;

            for (size_t i = 0; i != motion.pointer_count; ++i)
            {
                motion.pointer_coordinates[i].x += area.top_left.x.as_float();
                motion.pointer_coordinates[i].y += area.top_left.y.as_float();
            }

            auto pev = mir_input_event_get_pointer_event(iev);
            auto x = mir_pointer_event_axis_value(pev, mir_pointer_axis_x);
            auto y = mir_pointer_event_axis_value(pev, mir_pointer_axis_y);
            cursor_listener->cursor_moved_to(x, y);
        }

        dispatcher->dispatch(my_event);
    }
    else
    {
        dispatcher->dispatch(event);
    }
}
