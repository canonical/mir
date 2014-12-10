/*
 * Copyright © 2013-2014 Canonical Ltd.
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

#include "server_example_fullscreen_placement_strategy.h"

#include "mir/server.h"
#include "mir/options/option.h"
#include "mir/scene/surface_creation_parameters.h"
#include "mir/shell/display_layout.h"
#include "mir/geometry/rectangle.h"

namespace me = mir::examples;
namespace ms = mir::scene;
namespace msh = mir::shell;

///\example server_example_fullscreen_placement_strategy.cpp
/// Demonstrate a custom placement strategy (that fullscreens surfaces)

me::FullscreenPlacementStrategy::FullscreenPlacementStrategy(
    std::shared_ptr<msh::DisplayLayout> const& display_layout)
  : display_layout(display_layout)
{
}

ms::SurfaceCreationParameters me::FullscreenPlacementStrategy::place(ms::Session const& /* session */,
    ms::SurfaceCreationParameters const& request_parameters)
{
    auto placed_parameters = request_parameters;

    geometry::Rectangle rect{request_parameters.top_left, request_parameters.size};
    display_layout->size_to_output(rect);
    placed_parameters.size = rect.size;

    return placed_parameters;
}

void me::add_fullscreen_option_to(Server& server)
{
    server.add_configuration_option("fullscreen-surfaces", "Make all surfaces fullscreen", mir::OptionType::null);
    server.override_the_placement_strategy([&]()
        -> std::shared_ptr<ms::PlacementStrategy>
        {
            if (server.get_options()->is_set("fullscreen-surfaces"))
                return std::make_shared<me::FullscreenPlacementStrategy>(server.the_shell_display_layout());
            else
                return std::shared_ptr<ms::PlacementStrategy>{};
        });
}
