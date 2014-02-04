/*
 * Copyright © 2012 Canonical Ltd.
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
#include "surface_data.h"
#include "mir/compositor/buffer_stream.h"
#include "mir/input/input_channel.h"
#include "mir/graphics/buffer.h"

#include "mir/scene/scene_report.h"

#include <boost/throw_exception.hpp>

#include <stdexcept>

namespace mc = mir::compositor;
namespace ms = mir::scene;
namespace mg = mir::graphics;
namespace mi = mir::input;
namespace geom = mir::geometry;

ms::BasicSurface::BasicSurface(
    std::shared_ptr<SurfaceData> const& surface_data,
    std::shared_ptr<mc::BufferStream> const& buffer_stream,
    std::shared_ptr<input::InputChannel> const& input_channel,
    std::shared_ptr<SceneReport> const& report) :
    surface_data(surface_data),
    surface_buffer_stream(buffer_stream),
    server_input_channel(input_channel),
    report(report)
{
    report->surface_created(this);
}

void ms::BasicSurface::force_requests_to_complete()
{
    surface_buffer_stream->force_requests_to_complete();
}

ms::BasicSurface::~BasicSurface()
{
    report->surface_deleted(this);
}

std::shared_ptr<mc::BufferStream> ms::BasicSurface::buffer_stream() const
{
    return surface_buffer_stream;
}

std::shared_ptr<mc::CompositingCriteria> ms::BasicSurface::compositing_criteria()
{
    return surface_data;
}

std::string const& ms::BasicSurface::name() const
{
    return surface_data->name();
}

void ms::BasicSurface::move_to(geometry::Point const& top_left)
{
    surface_data->move_to(top_left);
}

void ms::BasicSurface::set_rotation(float degrees, glm::vec3 const& axis)
{
    surface_data->apply_rotation(degrees, axis);
}

float ms::BasicSurface::alpha() const
{
    return surface_data->alpha();
}

void ms::BasicSurface::set_alpha(float alpha_v)
{
    surface_data->apply_alpha(alpha_v);
}

void ms::BasicSurface::set_hidden(bool hide)
{
    surface_data->set_hidden(hide);
}

geom::Point ms::BasicSurface::top_left() const
{
    return surface_data->position();
}

mir::geometry::Size ms::BasicSurface::size() const
{
    return surface_data->size();
}

MirPixelFormat ms::BasicSurface::pixel_format() const
{
    return surface_buffer_stream->get_stream_pixel_format();
}

void ms::BasicSurface::swap_buffers(graphics::Buffer*& buffer)
{
    bool const posting{!!buffer};

    surface_buffer_stream->swap_client_buffers(buffer);

    if (posting)
    {
        surface_data->frame_posted();
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

std::shared_ptr<mi::Surface> ms::BasicSurface::input_surface() const
{
    return surface_data;
}

void ms::BasicSurface::set_input_region(std::vector<geom::Rectangle> const& input_rectangles)
{
    surface_data->set_input_region(input_rectangles);
}

bool ms::BasicSurface::resize(geom::Size const& size)
{
    if (size == this->size())
        return false;

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
    surface_data->resize(size);

    return true;
}
