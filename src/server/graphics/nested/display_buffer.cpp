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
#include "host_stream.h"
#include "host_surface_spec.h"
#include "native_buffer.h"
#include "mir/graphics/pixel_format_utils.h"
#include "mir/graphics/buffer.h"
#include "mir/graphics/egl_error.h"
#include "mir/events/event_private.h"

#include <sstream>
#include <boost/throw_exception.hpp>
#include <stdexcept>

namespace mg = mir::graphics;
namespace mgn = mir::graphics::nested;
namespace geom = mir::geometry;

namespace
{
std::shared_ptr<mgn::HostStream> create_host_stream(
    mgn::HostConnection& connection,
    mg::DisplayConfigurationOutput const& output)
{
    mg::BufferProperties properties(output.extents().size, output.current_format, mg::BufferUsage::hardware);
    return connection.create_stream(properties);
}

std::shared_ptr<mgn::HostSurface> create_host_surface(
    mgn::HostConnection& connection,
    std::shared_ptr<mgn::HostStream> const& host_stream,
    mg::DisplayConfigurationOutput const& output)
{
    std::ostringstream surface_title;
    surface_title << "Mir nested display for output #" << output.id.as_value();
    mg::BufferProperties properties(output.extents().size, output.current_format, mg::BufferUsage::hardware);
    return connection.create_surface(
        host_stream, mir::geometry::Displacement{0, 0}, properties,
        surface_title.str().c_str(), static_cast<uint32_t>(output.id.as_value()));
}
}

mgn::detail::DisplayBuffer::DisplayBuffer(
    EGLDisplayHandle const& egl_display,
    mg::DisplayConfigurationOutput best_output,
    std::shared_ptr<HostConnection> const& host_connection) :
    egl_display(egl_display),
    host_stream{create_host_stream(*host_connection, best_output)},
    host_surface{create_host_surface(*host_connection, host_stream, best_output)},
    host_connection{host_connection},
    host_chain{nullptr},
    egl_config{egl_display.choose_windowed_config(best_output.current_format)},
    egl_context{egl_display, eglCreateContext(egl_display, egl_config, egl_display.egl_context(), nested_egl_context_attribs)},
    area{best_output.extents()},
    egl_surface{egl_display, host_stream->egl_native_window(), egl_config}, 
    content{BackingContent::stream}
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

void mgn::detail::DisplayBuffer::swap_buffers()
{
    if (content != BackingContent::stream)
    {
        auto spec = host_connection->create_surface_spec();
        spec->add_stream(*host_stream, geom::Displacement{0,0});
        content = BackingContent::stream;
        host_surface->apply_spec(*spec);
    }
    eglSwapBuffers(egl_display, egl_surface);
}

void mgn::detail::DisplayBuffer::bind()
{
}

bool mgn::detail::DisplayBuffer::post_renderables_if_optimizable(RenderableList const& list)
{
    if (list.empty() ||
        (list.back()->screen_position() != area) ||
        (list.back()->alpha() != 1.0f) ||
        (list.back()->shaped()) ||
        (list.back()->transformation() != identity))
    {
        return false;
    }

    auto passthrough_buffer = list.back()->buffer();
    auto nested_buffer = dynamic_cast<mgn::NativeBuffer*>(passthrough_buffer->native_buffer_handle().get());
    if (!nested_buffer)
        return false;

    if (!host_chain)
        host_chain = host_connection->create_chain();

    host_chain->submit_buffer(passthrough_buffer);

    if (content != BackingContent::chain)
    {
        auto spec = host_connection->create_surface_spec();
        spec->add_chain(*host_chain, geom::Displacement{0,0}, passthrough_buffer->size());
        content = BackingContent::chain;
        host_surface->apply_spec(*spec);
    }

    return true;
}

MirOrientation mgn::detail::DisplayBuffer::orientation() const
{
    /*
     * Always normal orientation. The real rotation is handled by the
     * native display.
     */
    return mir_orientation_normal;
}

MirMirrorMode mgn::detail::DisplayBuffer::mirror_mode() const
{
    return mir_mirror_mode_none;
}

mgn::detail::DisplayBuffer::~DisplayBuffer() noexcept
{
    printf("AAAA\n");
    host_surface->set_event_handler(nullptr, nullptr);
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
    if (mir_event_get_type(&event) == mir_event_type_input ||
        mir_event_get_type(&event) == mir_event_type_input_device_state)
        host_connection->emit_input_event(event, area);
}

mg::NativeDisplayBuffer* mgn::detail::DisplayBuffer::native_display_buffer()
{
    return this;
}
