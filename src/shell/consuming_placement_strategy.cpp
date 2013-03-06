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
 * You should have received a copy of the GNU Lesser General Public License
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
    // We would like to try to fill placement requests
    auto placed_parameters = request_parameters;

    auto input_width = request_parameters.size.width.as_uint32_t();
    auto input_height = request_parameters.size.height.as_uint32_t();
    auto view_area = display_area->view_area();

    // If we have a size request of course we will attempt to fill it
    // (consuming-mode.feature: l12)
    //
    // TODO: It seems a little strange that we allow one dimension to be 0
    // but this does not seem like the right place to correct for that.
    if (input_width != 0 || input_height != 0)
    {
        // However, we should clip the request to the display size
        // (consuming-mode.feature: l21)
        placed_parameters.size.width = geom::Width{std::min(input_width, view_area.size.width.as_uint32_t())};
        placed_parameters.size.height = geom::Height{std::min(input_height, view_area.size.height.as_uint32_t())};
    }
    // Otherwise we consume the entire viewable area
    // (consuming-mode.feature: l5)
    else
    {
        placed_parameters.size = view_area.size;
    }
    
    return placed_parameters;
}
