/*
 * Copyright © 2014-2015 Canonical Ltd.
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

#include "mir_test_framework/declarative_placement_window_manage_policy.h"
#include "mir/scene/surface_creation_parameters.h"

namespace ms = mir::scene;

namespace mtf = mir_test_framework;

mtf::DeclarativePlacementWindowManagerPolicy::DeclarativePlacementWindowManagerPolicy(
    Tools* const tools,
    SurfaceGeometries const& positions_by_name,
    SurfaceDepths const& depths_by_name,
    std::shared_ptr<mir::shell::DisplayLayout> const& display_layout) :
    mir::shell::CanonicalWindowManagerPolicy{tools, display_layout},
    surface_geometries_by_name{positions_by_name},
    surface_depths_by_name{depths_by_name}
{
}

ms::SurfaceCreationParameters mtf::DeclarativePlacementWindowManagerPolicy::handle_place_new_surface(
    std::shared_ptr<mir::scene::Session> const& /*session*/,
    ms::SurfaceCreationParameters const& request_parameters)
{
    auto placed = request_parameters;

    auto const& name = request_parameters.name;
    
    if (surface_geometries_by_name.find(name) != surface_geometries_by_name.end())
    {
        auto const& geometry = surface_geometries_by_name.at(name);
        placed.top_left = geometry.top_left;
        placed.size = geometry.size;        
    }
    if (surface_depths_by_name.find(name) != surface_depths_by_name.end())
    {
        placed.depth = surface_depths_by_name.at(name);
    }

    return placed;
}



