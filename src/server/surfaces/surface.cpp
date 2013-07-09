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
#include "mir/surfaces/surface_info.h"
#include "mir/input/surface_info.h"
#include "mir/surfaces/buffer_stream.h"
#include "mir/input/input_channel.h"
#include "mir/compositor/buffer.h"
#include "mir/graphics/surface_info.h"

#include <boost/throw_exception.hpp>

#include <stdexcept>

namespace mc = mir::compositor;
namespace ms = mir::surfaces;
namespace mg = mir::graphics;
namespace mi = mir::input;
namespace geom = mir::geometry;

ms::Surface::Surface(
    std::shared_ptr<ms::SurfaceInfoController> const& basic_info,
    std::shared_ptr<mg::SurfaceInfoController> const& graphics_info,
    std::shared_ptr<ms::BufferStream> const& buffer_stream,
    std::shared_ptr<mi::SurfaceInfoController> const& input_info,
    std::shared_ptr<input::InputChannel> const& input_channel)
    : basic_info(basic_info),
      surface_input_info(input_info),
      gfx_info(graphics_info),
      surface_buffer_stream(buffer_stream),
      server_input_channel(input_channel)
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

std::shared_ptr<mg::SurfaceInfo> ms::Surface::graphics_info()
{
    return gfx_info;
}

std::string const& ms::Surface::name() const
{
    return basic_info->name();
}

void ms::Surface::move_to(geometry::Point const& top_left)
{
    basic_info->set_top_left(top_left);
}

void ms::Surface::set_rotation(float degrees, glm::vec3 const& axis)
{
    basic_info->apply_rotation(degrees, axis);
}

void ms::Surface::set_alpha(float alpha_v)
{
    gfx_info->apply_alpha(alpha_v);
}

void ms::Surface::set_hidden(bool hide)
{
    gfx_info->set_hidden(hide);
}

geom::Point ms::Surface::top_left() const
{
    return basic_info->size_and_position().top_left;
}

mir::geometry::Size ms::Surface::size() const
{
    return basic_info->size_and_position().size;
}

//note: not sure the surface should be aware of pixel format. might be something that the
//texture (which goes to compositor should be aware of though
//todo: kdub remove
geom::PixelFormat ms::Surface::pixel_format() const
{
    return surface_buffer_stream->get_stream_pixel_format();
}

std::shared_ptr<mc::Buffer> ms::Surface::advance_client_buffer()
{
    flag_for_render();
    return surface_buffer_stream->secure_client_buffer();
}

void ms::Surface::allow_framedropping(bool allow)
{
    surface_buffer_stream->allow_framedropping(allow);
}

std::shared_ptr<mc::Buffer> ms::Surface::compositor_buffer() const
{
    return surface_buffer_stream->lock_back_buffer();
}

//*TODO this is just used in example code, could be private
void ms::Surface::flag_for_render()
{
    gfx_info->frame_posted();
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

std::shared_ptr<mi::SurfaceInfo> ms::Surface::input_info() const
{
    return surface_input_info;
}

void ms::Surface::set_input_region(std::vector<geom::Rectangle> const& input_rectangles)
{
    surface_input_info->set_input_region(input_rectangles);
}
