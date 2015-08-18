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

#include "mir/scene/surface_creation_parameters.h"

namespace mg = mir::graphics;
namespace ms = mir::scene;
namespace mi = mir::input;
namespace mf = mir::frontend;
namespace geom = mir::geometry;

ms::SurfaceCreationParameters::SurfaceCreationParameters()
    : buffer_usage{mg::BufferUsage::undefined},
      pixel_format{mir_pixel_format_invalid},
      depth{0}, input_mode{mi::InputReceptionMode::normal}
{
}

ms::SurfaceCreationParameters& ms::SurfaceCreationParameters::of_name(std::string const& new_name)
{
    name = new_name;
    return *this;
}


ms::SurfaceCreationParameters& ms::SurfaceCreationParameters::of_size(
        geometry::Size new_size)
{
    size = new_size;

    return *this;
}

ms::SurfaceCreationParameters& ms::SurfaceCreationParameters::of_size(
    geometry::Width::ValueType width,
    geometry::Height::ValueType height)
{
    return of_size({width, height});
}

ms::SurfaceCreationParameters& ms::SurfaceCreationParameters::of_position(geometry::Point const& new_top_left)
{
    top_left = new_top_left;

    return *this;
}

ms::SurfaceCreationParameters& ms::SurfaceCreationParameters::of_buffer_usage(
        mg::BufferUsage new_buffer_usage)
{
    buffer_usage = new_buffer_usage;

    return *this;
}

ms::SurfaceCreationParameters& ms::SurfaceCreationParameters::of_pixel_format(
    MirPixelFormat new_pixel_format)
{
    pixel_format = new_pixel_format;

    return *this;
}

ms::SurfaceCreationParameters& ms::SurfaceCreationParameters::of_depth(
    scene::DepthId const& new_depth)
{
    depth = new_depth;

    return *this;
}

ms::SurfaceCreationParameters& ms::SurfaceCreationParameters::with_input_mode(input::InputReceptionMode const& new_mode)
{
    input_mode = new_mode;

    return *this;
}

ms::SurfaceCreationParameters& ms::SurfaceCreationParameters::with_output_id(
    graphics::DisplayConfigurationOutputId const& new_output_id)
{
    output_id = new_output_id;

    return *this;
}


ms::SurfaceCreationParameters& ms::SurfaceCreationParameters::of_type(MirSurfaceType the_type)
{
    type = the_type;
    return *this;
}

ms::SurfaceCreationParameters& ms::SurfaceCreationParameters::with_state(MirSurfaceState the_state)
{
    state = the_state;
    return *this;
}

ms::SurfaceCreationParameters& ms::SurfaceCreationParameters::with_preferred_orientation(MirOrientationMode mode)
{
    preferred_orientation = mode;
    return *this;
}

ms::SurfaceCreationParameters& ms::SurfaceCreationParameters::with_parent_id(mf::SurfaceId const& id)
{
    parent_id = id;
    return *this;
}

ms::SurfaceCreationParameters& ms::SurfaceCreationParameters::with_aux_rect(geometry::Rectangle const& rect)
{
    aux_rect = rect;
    return *this;
}

ms::SurfaceCreationParameters& ms::SurfaceCreationParameters::with_edge_attachment(MirEdgeAttachment edge)
{
    edge_attachment = edge;
    return *this;
}

ms::SurfaceCreationParameters& ms::SurfaceCreationParameters::with_buffer_stream(mf::BufferStreamId const& id)
{
    content_id = id;
    return *this;
}

bool ms::operator==(
    const SurfaceCreationParameters& lhs,
    const ms::SurfaceCreationParameters& rhs)
{
    return lhs.name == rhs.name &&
        lhs.size == rhs.size &&
        lhs.top_left == rhs.top_left &&
        lhs.buffer_usage == rhs.buffer_usage &&
        lhs.pixel_format == rhs.pixel_format &&
        lhs.depth == rhs.depth &&
        lhs.input_mode == rhs.input_mode &&
        lhs.output_id == rhs.output_id &&
        lhs.state == rhs.state &&
        lhs.type == rhs.type &&
        lhs.preferred_orientation == rhs.preferred_orientation &&
        lhs.parent_id == rhs.parent_id &&
        lhs.content_id == rhs.content_id;
}

bool ms::operator!=(
    const SurfaceCreationParameters& lhs,
    const ms::SurfaceCreationParameters& rhs)
{
    return !(lhs == rhs);
}

ms::SurfaceCreationParameters ms::a_surface()
{
    return SurfaceCreationParameters();
}
