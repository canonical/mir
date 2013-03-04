/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
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

#include "mir/sessions/surface_creation_parameters.h"

namespace mc = mir::compositor;
namespace msess = mir::sessions;
namespace geom = mir::geometry;

msess::SurfaceCreationParameters::SurfaceCreationParameters()
    : name(), size(), buffer_usage(mc::BufferUsage::undefined),
      pixel_format(geom::PixelFormat::invalid)
{
}

msess::SurfaceCreationParameters& msess::SurfaceCreationParameters::of_name(std::string const& new_name)
{
    name = new_name;
    return *this;
}


msess::SurfaceCreationParameters& msess::SurfaceCreationParameters::of_size(
        geometry::Size new_size)
{
    size = new_size;

    return *this;
}

msess::SurfaceCreationParameters& msess::SurfaceCreationParameters::of_size(
    geometry::Width::ValueType width,
    geometry::Height::ValueType height)
{
    return of_size(geometry::Size(geometry::Width(width), geometry::Height(height)));
}

msess::SurfaceCreationParameters& msess::SurfaceCreationParameters::of_buffer_usage(
        mc::BufferUsage new_buffer_usage)
{
    buffer_usage = new_buffer_usage;

    return *this;
}

msess::SurfaceCreationParameters& msess::SurfaceCreationParameters::of_pixel_format(
    geom::PixelFormat new_pixel_format)
{
    pixel_format = new_pixel_format;

    return *this;
}

bool msess::operator==(
    const SurfaceCreationParameters& lhs,
    const msess::SurfaceCreationParameters& rhs)
{
    return lhs.size == rhs.size &&
           lhs.buffer_usage == rhs.buffer_usage &&
           lhs.pixel_format == rhs.pixel_format;
}

bool msess::operator!=(
    const SurfaceCreationParameters& lhs,
    const msess::SurfaceCreationParameters& rhs)
{
    return !(lhs == rhs);
}

msess::SurfaceCreationParameters msess::a_surface()
{
    return SurfaceCreationParameters();
}
