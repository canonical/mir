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

#include "mir/shell/surface_creation_parameters.h"

namespace mg = mir::graphics;
namespace msh = mir::shell;
namespace mi = mir::input;
namespace geom = mir::geometry;

msh::SurfaceCreationParameters::SurfaceCreationParameters()
    : name(), size(), top_left(), buffer_usage(mg::BufferUsage::undefined),
      pixel_format(geom::PixelFormat::invalid),
      depth{0}, input_mode(mi::InputReceptionMode::normal)
{
}

msh::SurfaceCreationParameters& msh::SurfaceCreationParameters::of_name(std::string const& new_name)
{
    name = new_name;
    return *this;
}


msh::SurfaceCreationParameters& msh::SurfaceCreationParameters::of_size(
        geometry::Size new_size)
{
    size = new_size;

    return *this;
}

msh::SurfaceCreationParameters& msh::SurfaceCreationParameters::of_size(
    geometry::Width::ValueType width,
    geometry::Height::ValueType height)
{
    return of_size({width, height});
}

msh::SurfaceCreationParameters& msh::SurfaceCreationParameters::of_position(geometry::Point const& new_top_left)
{
    top_left = new_top_left;
    
    return *this;
}

msh::SurfaceCreationParameters& msh::SurfaceCreationParameters::of_buffer_usage(
        mg::BufferUsage new_buffer_usage)
{
    buffer_usage = new_buffer_usage;

    return *this;
}

msh::SurfaceCreationParameters& msh::SurfaceCreationParameters::of_pixel_format(
    geom::PixelFormat new_pixel_format)
{
    pixel_format = new_pixel_format;

    return *this;
}

msh::SurfaceCreationParameters& msh::SurfaceCreationParameters::of_depth(
    scene::DepthId const& new_depth)
{
    depth = new_depth;
    
    return *this;
}

msh::SurfaceCreationParameters& msh::SurfaceCreationParameters::with_input_mode(input::InputReceptionMode const& new_mode)
{
    input_mode = new_mode;
    
    return *this;
}

msh::SurfaceCreationParameters& msh::SurfaceCreationParameters::with_output_id(
    graphics::DisplayConfigurationOutputId const& new_output_id)
{
    output_id = new_output_id;

    return *this;
}

bool msh::operator==(
    const SurfaceCreationParameters& lhs,
    const msh::SurfaceCreationParameters& rhs)
{
    return lhs.name == rhs.name && 
        lhs.size == rhs.size &&
        lhs.top_left == rhs.top_left &&
        lhs.buffer_usage == rhs.buffer_usage &&
        lhs.pixel_format == rhs.pixel_format &&
        lhs.depth == rhs.depth &&
        lhs.input_mode == rhs.input_mode &&
        lhs.output_id == lhs.output_id;
}

bool msh::operator!=(
    const SurfaceCreationParameters& lhs,
    const msh::SurfaceCreationParameters& rhs)
{
    return !(lhs == rhs);
}

msh::SurfaceCreationParameters msh::a_surface()
{
    return SurfaceCreationParameters();
}
