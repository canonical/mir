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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#include "mir/shell/consuming_placement_strategy.h"
#include "mir/shell/surface_creation_parameters.h"
#include "mir/shell/display_layout.h"
#include "mir/geometry/rectangle.h"

#include "mir_toolkit/client_types.h"

#include <algorithm>

namespace msh = mir::shell;
namespace geom = mir::geometry;

msh::ConsumingPlacementStrategy::ConsumingPlacementStrategy(
    std::shared_ptr<msh::DisplayLayout> const& display_layout)
    : display_layout(display_layout)
{
}

msh::SurfaceCreationParameters msh::ConsumingPlacementStrategy::place(
    msh::Session const& /* session */,
    msh::SurfaceCreationParameters const& request_parameters)
{
    mir::graphics::DisplayConfigurationOutputId const output_id_invalid{
        mir_display_output_id_invalid};
    auto placed_parameters = request_parameters;

    geom::Rectangle rect{request_parameters.top_left, request_parameters.size};

    if (request_parameters.output_id != output_id_invalid)
    {
        display_layout->place_in_output(request_parameters.output_id, rect);
    }
    else if (request_parameters.size.width > geom::Width{0} &&
             request_parameters.size.height > geom::Height{0})
    {
        display_layout->clip_to_output(rect);
    }
    else
    {
        display_layout->size_to_output(rect);
    }

    placed_parameters.top_left = rect.top_left;
    placed_parameters.size = rect.size;

    return placed_parameters;
}
