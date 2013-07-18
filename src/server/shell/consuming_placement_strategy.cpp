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
#include "mir/shell/surface_boundaries.h"
#include "mir/geometry/rectangle.h"

#include <algorithm>

namespace msh = mir::shell;
namespace geom = mir::geometry;

msh::ConsumingPlacementStrategy::ConsumingPlacementStrategy(
    std::shared_ptr<msh::SurfaceBoundaries> const& surface_boundaries)
    : surface_boundaries(surface_boundaries)
{
}

msh::SurfaceCreationParameters msh::ConsumingPlacementStrategy::place(msh::SurfaceCreationParameters const& request_parameters)
{
    auto placed_parameters = request_parameters;

    geom::Rectangle rect{request_parameters.top_left, request_parameters.size};

    if (request_parameters.size.width != geom::Width{0} &&
        request_parameters.size.height != geom::Height{0})
    {
        surface_boundaries->clip_to_screen(rect);
    }
    else
    {
        surface_boundaries->make_fullscreen(rect);
    }

    placed_parameters.top_left = rect.top_left;
    placed_parameters.size = rect.size;

    return placed_parameters;
}
