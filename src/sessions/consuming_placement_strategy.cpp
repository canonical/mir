/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
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

#include "mir/sessions/consuming_placement_strategy.h"
#include "mir/graphics/viewable_area.h"

#include <algorithm>

namespace msess = mir::sessions;
namespace mg = mir::graphics;
namespace ms = mir::surfaces;
namespace geom = mir::geometry;

msess::ConsumingPlacementStrategy::ConsumingPlacementStrategy(std::shared_ptr<mg::ViewableArea> const& display_area)
    : display_area(display_area)
{
}

void msess::ConsumingPlacementStrategy::place(ms::SurfaceCreationParameters const& request_parameters,
                                      ms::SurfaceCreationParameters &placed_parameters)
{
    placed_parameters = request_parameters;

    auto input_width = request_parameters.size.width.as_uint32_t();
    auto input_height = request_parameters.size.height.as_uint32_t();
    auto view_area = display_area->view_area();

    // If we have a request try and fill it. We may have to clip to 
    // the screen size though (consuming-mode.feature: l23).
    //
    // TODO: It seems a little strange that we allow one dimension to be 0
    // but this does not seem like the right place to correct for that.
    if (input_width != 0 || input_height != 0)
    {
        placed_parameters.size.width = geom::Width{std::min(input_width, view_area.size.width.as_uint32_t())};
        placed_parameters.size.height = geom::Height{std::min(input_height, view_area.size.height.as_uint32_t())};
        return;
    }
    
    // Otherwise default to display size (consuming-mode.feature: l5)
    placed_parameters.size = view_area.size;
}
