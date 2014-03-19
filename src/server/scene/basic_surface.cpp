/*
 * Copyright © 2012-2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by:
 *   Alan Griffiths <alan@octopull.co.uk>
 *   Thomas Voss <thomas.voss@canonical.com>
 */

#include "basic_surface.h"
#include "mir/compositor/buffer_stream.h"
#include "mir/frontend/event_sink.h"
#include "mir/input/input_channel.h"
#include "mir/shell/input_targeter.h"
#include "mir/graphics/buffer.h"

#include "mir/scene/scene_report.h"
#include "mir/shell/surface_configurator.h"

#define GLM_FORCE_RADIANS
#include <glm/gtc/matrix_transform.hpp>

#include <boost/throw_exception.hpp>

#include <stdexcept>
#include <cstring>

namespace mc = mir::compositor;
namespace ms = mir::scene;
namespace msh = mir::shell;
namespace mg = mir::graphics;
namespace mi = mir::input;
namespace geom = mir::geometry;

ms::BasicSurface::BasicSurface(
    frontend::SurfaceId id,
    std::string const& name,
    geometry::Rectangle rect,
    std::function<void()> change_cb,
    bool nonrectangular,
    std::shared_ptr<mc::BufferStream> const& buffer_stream,
    std::shared_ptr<input::InputChannel> const& input_channel,
    std::shared_ptr<frontend::EventSink> const& event_sink,
    std::shared_ptr<shell::SurfaceConfigurator> const& configurator,
    std::shared_ptr<SceneReport> const& report) :
    id(id),
    notify_change(change_cb),
    surface_name(name),
    surface_rect(rect),
    surface_alpha(1.0f),
    first_frame_posted(false),
    hidden(false),
    nonrectangular(nonrectangular),
    input_rectangles{surface_rect},
    surface_buffer_stream(buffer_stream),
    server_input_channel(input_channel),
    event_sink(event_sink),
    configurator(configurator),
    report(report),
    type_value(mir_surface_type_normal),
    state_value(mir_surface_state_restored)
{
    report->surface_created(this, surface_name);
}

void ms::BasicSurface::force_requests_to_complete()
{
    surface_buffer_stream->force_requests_to_complete();
}

ms::BasicSurface::~BasicSurface() noexcept
{
    report->surface_deleted(this, surface_name);
}

std::shared_ptr<mc::BufferStream> ms::BasicSurface::buffer_stream() const
{
    return surface_buffer_stream;
}

std::string ms::BasicSurface::name() const
{
    return surface_name;
}

void ms::BasicSurface::move_to(geometry::Point const& top_left)
{
    {
        std::unique_lock<std::mutex> lk(guard);
        surface_rect.top_left = top_left;
    }
    notify_change();
}

float ms::BasicSurface::alpha() const
{
    std::unique_lock<std::mutex> lk(guard);
    return surface_alpha;
}

void ms::BasicSurface::set_hidden(bool hide)
{
    {
        std::unique_lock<std::mutex> lk(guard);
        hidden = hide;
    }
    notify_change();
}

mir::geometry::Size ms::BasicSurface::size() const
{
    std::unique_lock<std::mutex> lk(guard);
    return surface_rect.size;
}

MirPixelFormat ms::BasicSurface::pixel_format() const
{
    return surface_buffer_stream->get_stream_pixel_format();
}

void ms::BasicSurface::swap_buffers(mg::Buffer* old_buffer, std::function<void(mg::Buffer* new_buffer)> complete)
{
    bool const posting{!!old_buffer};

    surface_buffer_stream->swap_client_buffers(old_buffer, complete);

    if (posting)
    {
        {
            std::unique_lock<std::mutex> lk(guard);
            first_frame_posted = true;
        }
        notify_change();
    }
}

void ms::BasicSurface::allow_framedropping(bool allow)
{
    surface_buffer_stream->allow_framedropping(allow);
}

std::shared_ptr<mg::Buffer> ms::BasicSurface::snapshot_buffer() const
{
    return surface_buffer_stream->lock_snapshot_buffer();
}

bool ms::BasicSurface::supports_input() const
{
    if (server_input_channel)
        return true;
    return false;
}

int ms::BasicSurface::client_input_fd() const
{
    if (!supports_input())
        BOOST_THROW_EXCEPTION(std::logic_error("Surface does not support input"));
    return server_input_channel->client_fd();
}

std::shared_ptr<mi::InputChannel> ms::BasicSurface::input_channel() const
{
    return server_input_channel;
}

void ms::BasicSurface::set_input_region(std::vector<geom::Rectangle> const& input_rectangles)
{
    std::unique_lock<std::mutex> lock(guard);
    this->input_rectangles = input_rectangles;
}

void ms::BasicSurface::resize(geom::Size const& size)
{
    if (size == this->size())
        return;

    if (size.width <= geom::Width{0} || size.height <= geom::Height{0})
    {
        BOOST_THROW_EXCEPTION(std::logic_error("Impossible resize requested"));
    }
    /*
     * Other combinations may still be invalid (like dimensions too big or
     * insufficient resources), but those are runtime and platform-specific, so
     * not predictable here. Such critical exceptions would arise from
     * the platform buffer allocator as a runtime_error via:
     */
    surface_buffer_stream->resize(size);

    // Now the buffer stream has successfully resized, update the state second;
    {
        std::unique_lock<std::mutex> lock(guard);
        surface_rect.size = size;
    }
    notify_change();
    MirEvent e;
    memset(&e, 0, sizeof e);
    e.type = mir_event_type_resize;
    e.resize.surface_id = id.as_value();
    e.resize.width = size.width.as_int();
    e.resize.height = size.height.as_int();
    event_sink->handle_event(e);
}

geom::Point ms::BasicSurface::top_left() const
{
    std::unique_lock<std::mutex> lk(guard);
    return surface_rect.top_left;
}

bool ms::BasicSurface::contains(geom::Point const& point) const
{
    std::unique_lock<std::mutex> lock(guard);
    if (hidden)
        return false;

    for (auto const& rectangle : input_rectangles)
    {
        if (rectangle.contains(point))
        {
            return true;
        }
    }
    return false;
}

void ms::BasicSurface::frame_posted()
{
    {
        std::unique_lock<std::mutex> lk(guard);
        first_frame_posted = true;
    }
    notify_change();
}

void ms::BasicSurface::set_alpha(float alpha)
{
    {
        std::unique_lock<std::mutex> lk(guard);
        surface_alpha = alpha;
    }
    notify_change();
}


void ms::BasicSurface::set_rotation(float degrees, glm::vec3 const& axis)
{
    {
        std::unique_lock<std::mutex> lk(guard);
        rotation_matrix = glm::rotate(glm::mat4(1.0f), glm::radians(degrees), axis);
    }
    notify_change();
}

glm::mat4 ms::BasicSurface::transformation() const
{
    std::unique_lock<std::mutex> lk(guard);

    // By default the only transformation implemented is rotation...
    return rotation_matrix;
}

bool ms::BasicSurface::should_be_rendered_in(geom::Rectangle const& rect) const
{
    std::unique_lock<std::mutex> lk(guard);

    if (hidden || !first_frame_posted)
        return false;

    return rect.overlaps(surface_rect);
}

bool ms::BasicSurface::shaped() const
{
    return nonrectangular;
}

std::shared_ptr<mg::Buffer>
    ms::BasicSurface::buffer(unsigned long frameno) const
{
    return buffer_stream()->lock_compositor_buffer(frameno);
}

bool ms::BasicSurface::alpha_enabled() const
{
    return shaped() || alpha() < 1.0f;
}

geom::Rectangle ms::BasicSurface::screen_position() const
{   // This would be more efficient to return a const reference
    return surface_rect;
}

int ms::BasicSurface::buffers_ready_for_compositor() const
{
    return surface_buffer_stream->buffers_ready_for_compositor();
}

void ms::BasicSurface::with_most_recent_buffer_do(
    std::function<void(mg::Buffer&)> const& exec)
{
    auto buf = snapshot_buffer();
    exec(*buf);
}


MirSurfaceType ms::BasicSurface::type() const
{
    return type_value;
}

bool ms::BasicSurface::set_type(MirSurfaceType t)
{
    bool valid = false;

    if (t >= 0 && t < mir_surface_types)
    {
        type_value = t;
        valid = true;
    }

    return valid;
}

MirSurfaceState ms::BasicSurface::state() const
{
    return state_value;
}

bool ms::BasicSurface::set_state(MirSurfaceState s)
{
    bool valid = false;

    if (s > mir_surface_state_unknown &&
        s < mir_surface_states)
    {
        state_value = s;
        valid = true;

        notify_attrib_change(mir_surface_attrib_state, s);
    }

    return valid;
}

void ms::BasicSurface::notify_attrib_change(MirSurfaceAttrib attrib, int value)
{
    MirEvent e;

    // This memset is not really required. However it does avoid some
    // harmless uninitialized memory reads that valgrind will complain
    // about, due to gaps in MirEvent.
    memset(&e, 0, sizeof e);

    e.type = mir_event_type_surface;
    e.surface.id = id.as_value();
    e.surface.attrib = attrib;
    e.surface.value = value;

    event_sink->handle_event(e);
}

void ms::BasicSurface::take_input_focus(std::shared_ptr<msh::InputTargeter> const& targeter)
{
    targeter->focus_changed(input_channel());
}

void ms::BasicSurface::raise(std::shared_ptr<ms::SurfaceRanker> const& /*controller*/)
{
    BOOST_THROW_EXCEPTION(std::logic_error("Need refactoring to implement here"));
}

int ms::BasicSurface::configure(MirSurfaceAttrib attrib, int value)
{
    int result = 0;
    bool allow_dropping = false;
    /*
     * TODO: In future, query the shell implementation for the subset of
     *       attributes/types it implements.
     */
    value = configurator->select_attribute_value(*this, attrib, value);
    switch (attrib)
    {
    case mir_surface_attrib_type:
        if (!set_type(static_cast<MirSurfaceType>(value)))
            BOOST_THROW_EXCEPTION(std::logic_error("Invalid surface "
                                                   "type."));
        result = type();
        break;
    case mir_surface_attrib_state:
        if (value != mir_surface_state_unknown &&
            !set_state(static_cast<MirSurfaceState>(value)))
            BOOST_THROW_EXCEPTION(std::logic_error("Invalid surface state."));
        result = state();
        break;
    case mir_surface_attrib_focus:
        notify_attrib_change(attrib, value);
        break;
    case mir_surface_attrib_swapinterval:
        allow_dropping = (value == 0);
        allow_framedropping(allow_dropping);
        result = value;
        break;
    default:
        BOOST_THROW_EXCEPTION(std::logic_error("Invalid surface "
                                               "attribute."));
        break;
    }

    configurator->attribute_set(*this, attrib, result);

    return result;
}

void ms::BasicSurface::hide()
{
    set_hidden(true);
}

void ms::BasicSurface::show()
{
    set_hidden(false);
}

