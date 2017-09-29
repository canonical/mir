/*
 * Copyright © 2014-2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
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
    miral::WindowManagerTools const& tools,
    SurfaceGeometries const& positions_by_name) :
    mir_test_framework::CanonicalWindowManagerPolicy{tools},
    surface_geometries_by_name{positions_by_name}
{
}

auto mtf::DeclarativePlacementWindowManagerPolicy::place_new_window(
    miral::ApplicationInfo const& /*app_info*/,
    miral::WindowSpecification const& request_parameters)
-> miral::WindowSpecification
{
    auto placed = request_parameters;

    auto const& name = request_parameters.name().value();

    if (surface_geometries_by_name.find(name) != surface_geometries_by_name.end())
    {
        auto const& geometry = surface_geometries_by_name.at(name);
        placed.top_left() = geometry.top_left;
        placed.size() = geometry.size;
    }

    return placed;
}



