/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
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
#include "buffer.h"

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
    std::shared_ptr<HostConnection> const& host_connection,
    mgn::PassthroughOption const option) :
    egl_display(egl_display),
    host_stream{create_host_stream(*host_connection, best_output)},
    host_surface{create_host_surface(*host_connection, host_stream, best_output)},
    host_connection{host_connection},
    host_chain{nullptr},
    egl_config{egl_display.choose_windowed_config(best_output.current_format)},
    egl_context{egl_display, eglCreateContext(egl_display, egl_config, egl_display.egl_context(), nested_egl_context_attribs)},
    area{best_output.extents()},
    egl_surface{egl_display, host_stream->egl_native_window(), egl_config},
    passthrough_option(option),
    content{BackingContent::stream},
    identity()
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
    eglSwapBuffers(egl_display, egl_surface);
    if (content != BackingContent::stream)
    {
        auto spec = host_connection->create_surface_spec();
        spec->add_stream(*host_stream, geom::Displacement{0,0}, area.size);
        content = BackingContent::stream;
        host_surface->apply_spec(*spec);
        //if the host_chain is not released, a buffer of the passthrough surface might get caught
        //up in the host server, resulting a drop in nbuffers available to the client
        host_chain = nullptr;
    }
}

void mgn::detail::DisplayBuffer::bind()
{
}

bool mgn::detail::DisplayBuffer::overlay(RenderableList const& list)
{
    if ((passthrough_option == mgn::PassthroughOption::disabled) ||
        list.empty())
    {
        //could not represent scene with subsurfaces
        return false;
    }

    auto const topmost = list.back();

    if ((topmost->screen_position() != area) ||
        (topmost->alpha() != 1.0f) ||
        (topmost->shaped()) ||
        (topmost->transformation() != identity))
    {
        //could not represent scene with subsurfaces
        return false;
    }

    auto passthrough_buffer = topmost->buffer();
    auto native = dynamic_cast<mgn::NativeBuffer*>(passthrough_buffer->native_buffer_handle().get());
    if (!native)
        return false;

    if (!host_chain)
        host_chain = host_connection->create_chain();

    {
        std::unique_lock<std::mutex> lk(mutex);
        SubmissionInfo submission_info{native->client_handle(), host_chain->handle()};
        auto submitted = submitted_buffers.find(submission_info);
        if ((submission_info != last_submitted) && (submitted != submitted_buffers.end()))
            BOOST_THROW_EXCEPTION(std::logic_error("cannot resubmit buffer that has not been returned by host server"));
        if ((submission_info == last_submitted) && (submitted != submitted_buffers.end()))
            return true;

        if (topmost->swap_interval() == 0)
            host_chain->set_submission_mode(mgn::SubmissionMode::dropping);
        else
            host_chain->set_submission_mode(mgn::SubmissionMode::queueing);

        submitted_buffers[submission_info] = passthrough_buffer;
        last_submitted = submission_info;
    }

    native->on_ownership_notification(
        std::bind(&mgn::detail::DisplayBuffer::release_buffer, this,
        native->client_handle(), host_chain->handle()));
    host_chain->submit_buffer(*native);

    if (content != BackingContent::chain)
    {
        auto spec = host_connection->create_surface_spec();
        spec->add_chain(*host_chain, geom::Displacement{0,0}, area.size);
        content = BackingContent::chain;
        host_surface->apply_spec(*spec);
    }
    return true;
}

void mgn::detail::DisplayBuffer::release_buffer(MirBuffer* b, MirPresentationChain *c)
{
    std::unique_lock<std::mutex> lk(mutex);
    auto buf = submitted_buffers.find(SubmissionInfo{b, c});
    if (buf != submitted_buffers.end())
        submitted_buffers.erase(buf);
}

glm::mat2 mgn::detail::DisplayBuffer::transformation() const
{
    return {};
}

mgn::detail::DisplayBuffer::~DisplayBuffer() noexcept
{
    for(auto& b : submitted_buffers)
    {
        auto n = dynamic_cast<mgn::NativeBuffer*>(b.second->native_buffer_handle().get());
        n->on_ownership_notification([]{});
    }
    host_surface->set_event_handler(nullptr, nullptr);
}

void mgn::detail::DisplayBuffer::event_thunk(
    MirWindow* /*surface*/,
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
