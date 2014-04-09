/*
 * Copyright © 2014 Canonical Ltd.
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

#include "mir/shell/surface_creation_parameters.h"

#include "mir_test_framework/declarative_placement_strategy.h"

namespace ms = mir::scene;
namespace msh = mir::shell;

namespace mtf = mir_test_framework;

mtf::DeclarativePlacementStrategy::DeclarativePlacementStrategy(
    std::shared_ptr<msh::PlacementStrategy> const& default_strategy,
    SurfaceGeometries const& positions, 
    SurfaceDepths const& depths)
    : default_strategy(default_strategy),
    surface_geometries_by_name(positions),
    surface_depths_by_name(depths)
{
}

msh::SurfaceCreationParameters mtf::DeclarativePlacementStrategy::place(ms::Session const& session, msh::SurfaceCreationParameters const& request_parameters)
{
    auto placed = default_strategy->place(session, request_parameters);

    auto const& name = request_parameters.name;
    
    if (surface_geometries_by_name.find(name) != surface_geometries_by_name.end())
    {
        auto const& geometry = surface_geometries_by_name[name];
        placed.top_left = geometry.top_left;
        placed.size = geometry.size;        
    }
    if (surface_depths_by_name.find(name) != surface_depths_by_name.end())
    {
        placed.depth = surface_depths_by_name[name];
    }

    return placed;
}



