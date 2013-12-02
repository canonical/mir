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

#ifndef MIR_SHELL_SURFACE_CREATION_PARAMETERS_H_
#define MIR_SHELL_SURFACE_CREATION_PARAMETERS_H_

#include "mir/geometry/pixel_format.h"
#include "mir/geometry/point.h"
#include "mir/geometry/size.h"
#include "mir/graphics/buffer_properties.h"
#include "mir/graphics/display_configuration.h"
#include "mir/scene/depth_id.h"
#include "mir/input/input_reception_mode.h"

#include <memory>
#include <string>

namespace mir
{
namespace shell
{

struct SurfaceCreationParameters
{
    SurfaceCreationParameters();

    SurfaceCreationParameters& of_name(std::string const& new_name);

    SurfaceCreationParameters& of_size(geometry::Size new_size);

    SurfaceCreationParameters& of_size(geometry::Width::ValueType width, geometry::Height::ValueType height);

    SurfaceCreationParameters& of_position(geometry::Point const& top_left);

    SurfaceCreationParameters& of_buffer_usage(graphics::BufferUsage new_buffer_usage);

    SurfaceCreationParameters& of_pixel_format(geometry::PixelFormat new_pixel_format);

    SurfaceCreationParameters& of_depth(scene::DepthId const& new_depth);

    SurfaceCreationParameters& with_input_mode(input::InputReceptionMode const& new_mode);

    SurfaceCreationParameters& with_output_id(graphics::DisplayConfigurationOutputId const& output_id);

    std::string name;
    geometry::Size size;
    geometry::Point top_left;
    graphics::BufferUsage buffer_usage;
    geometry::PixelFormat pixel_format;
    scene::DepthId depth;
    input::InputReceptionMode input_mode;
    graphics::DisplayConfigurationOutputId output_id;
};

bool operator==(const SurfaceCreationParameters& lhs, const SurfaceCreationParameters& rhs);
bool operator!=(const SurfaceCreationParameters& lhs, const SurfaceCreationParameters& rhs);

SurfaceCreationParameters a_surface();
}
}

#endif /* MIR_SHELL_SURFACE_CREATION_PARAMETERS_H_ */
