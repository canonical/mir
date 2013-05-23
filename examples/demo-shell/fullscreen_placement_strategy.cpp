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

#include "fullscreen_placement_strategy.h"

#include "mir/shell/surface_creation_parameters.h"
#include "mir/graphics/viewable_area.h"

namespace me = mir::examples;
namespace msh = mir::shell;
namespace mg = mir::graphics;

me::FullscreenPlacementStrategy::FullscreenPlacementStrategy(std::shared_ptr<mg::ViewableArea> const& display_area)
  : display_area(display_area)
{
}

msh::SurfaceCreationParameters me::FullscreenPlacementStrategy::place(msh::SurfaceCreationParameters const& request_parameters)
{
    auto placed_parameters = request_parameters;

    placed_parameters.size = display_area->view_area().size;

    return placed_parameters;
}
