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
#include "mir/shell/display_layout.h"
#include "mir/geometry/rectangle.h"

namespace me = mir::examples;
namespace msh = mir::shell;

me::FullscreenPlacementStrategy::FullscreenPlacementStrategy(
    std::shared_ptr<msh::DisplayLayout> const& display_layout)
  : display_layout(display_layout)
{
}

msh::SurfaceCreationParameters me::FullscreenPlacementStrategy::place(msh::Session const& /* session */,
    msh::SurfaceCreationParameters const& request_parameters)
{
    auto placed_parameters = request_parameters;

    geometry::Rectangle rect{request_parameters.top_left, request_parameters.size};
    display_layout->size_to_output(rect);
    placed_parameters.size = rect.size;

    return placed_parameters;
}
