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
#include "mir/graphics/viewable_area.h"

#include <algorithm>

namespace msh = mir::shell;
namespace mg = mir::graphics;
namespace geom = mir::geometry;

msh::ConsumingPlacementStrategy::ConsumingPlacementStrategy(std::shared_ptr<mg::ViewableArea> const& display_area)
    : display_area(display_area)
{
}

msh::SurfaceCreationParameters msh::ConsumingPlacementStrategy::place(msh::SurfaceCreationParameters const& request_parameters)
{
    auto placed_parameters = request_parameters;

    auto input_width = request_parameters.size.width.as_uint32_t();
    auto input_height = request_parameters.size.height.as_uint32_t();
    auto view_area = display_area->view_area();

    if (input_width != 0 || input_height != 0)
    {
        placed_parameters.size.width = geom::Width{std::min(input_width, view_area.size.width.as_uint32_t())};
        placed_parameters.size.height = geom::Height{std::min(input_height, view_area.size.height.as_uint32_t())};
    }
    else
    {
        placed_parameters.size = view_area.size;
    }

    return placed_parameters;
}
