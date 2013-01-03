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

#include "mir/frontend/consuming_placement_strategy.h"
#include "mir/graphics/viewable_area.h"

namespace mf = mir::frontend;
namespace mg = mir::graphics;
namespace ms = mir::surfaces;

mf::ConsumingPlacementStrategy::ConsumingPlacementStrategy(std::shared_ptr<mg::ViewableArea> const& display_area)
    : display_area(display_area)
{
}

void mf::ConsumingPlacementStrategy::place(ms::SurfaceCreationParameters const& input_parameters,
                                      ms::SurfaceCreationParameters &output_parameters)
{
    output_parameters = input_parameters;

    // If we have a requested size use it. It seems a little strange to allow 0 as one dimension but 
    // it does not seem like this is the place to check for that.
    if (input_parameters.size.width.as_uint32_t() != 0 ||
        input_parameters.size.height.as_uint32_t() != 0)
    {
        return;
    }
    
    // Otherwise default to display size
    output_parameters.size = display_area->view_area().size;
}
