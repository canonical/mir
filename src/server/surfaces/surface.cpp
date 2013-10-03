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

#include "mir/surfaces/surface.h"
#include "surface_state.h"
#include "mir/surfaces/buffer_stream.h"
#include "mir/input/input_channel.h"
#include "mir/graphics/buffer.h"

#include <boost/throw_exception.hpp>

#include <stdexcept>

namespace mc = mir::compositor;
namespace ms = mir::surfaces;
namespace mg = mir::graphics;
namespace mi = mir::input;
namespace geom = mir::geometry;

ms::Surface::Surface(
    std::shared_ptr<ms::SurfaceState> const& state,
    std::shared_ptr<ms::BufferStream> const& buffer_stream,
    std::shared_ptr<input::InputChannel> const& input_channel)
    : surface_state(state),
      surface_buffer_stream(buffer_stream),
      server_input_channel(input_channel),
      surface_in_startup(true)
{
}

void ms::Surface::force_requests_to_complete()
{
    surface_buffer_stream->force_requests_to_complete();
}

ms::Surface::~Surface()
{
}

std::shared_ptr<ms::BufferStream> ms::Surface::buffer_stream() const
{
    return surface_buffer_stream;
}

std::shared_ptr<mc::CompositingCriteria> ms::Surface::compositing_criteria()
{
    return surface_state;
}

std::string const& ms::Surface::name() const
{
    return surface_state->name();
}

void ms::Surface::move_to(geometry::Point const& top_left)
{
    surface_state->move_to(top_left);
}

void ms::Surface::set_rotation(float degrees, glm::vec3 const& axis)
{
    surface_state->apply_rotation(degrees, axis);
}

void ms::Surface::set_alpha(float alpha_v)
{
    surface_state->apply_alpha(alpha_v);
}

void ms::Surface::set_hidden(bool hide)
{
    surface_state->set_hidden(hide);
}

geom::Point ms::Surface::top_left() const
{
    return surface_state->position();
}

mir::geometry::Size ms::Surface::size() const
{
    return surface_state->size();
}

geom::PixelFormat ms::Surface::pixel_format() const
{
    return surface_buffer_stream->get_stream_pixel_format();
}

std::shared_ptr<mg::Buffer> ms::Surface::advance_client_buffer()
{
    if (surface_in_startup)
    {
        surface_in_startup = false;
    }
    else
    {
        flag_for_render();
    }

    return surface_buffer_stream->secure_client_buffer();
}

void ms::Surface::allow_framedropping(bool allow)
{
    surface_buffer_stream->allow_framedropping(allow);
}

std::shared_ptr<mg::Buffer> ms::Surface::snapshot_buffer() const
{
    return surface_buffer_stream->lock_snapshot_buffer();
}

//TODO: this is just used in example code, could be private
void ms::Surface::flag_for_render()
{
    surface_state->frame_posted();
}

bool ms::Surface::supports_input() const
{
    if (server_input_channel)
        return true;
    return false;
}

int ms::Surface::client_input_fd() const
{
    if (!supports_input())
        BOOST_THROW_EXCEPTION(std::logic_error("Surface does not support input"));
    return server_input_channel->client_fd();
}

std::shared_ptr<mi::InputChannel> ms::Surface::input_channel() const
{
    return server_input_channel;
}

std::shared_ptr<mi::Surface> ms::Surface::input_surface() const
{
    return surface_state;
}

void ms::Surface::set_input_region(std::vector<geom::Rectangle> const& input_rectangles)
{
    surface_state->set_input_region(input_rectangles);
}
