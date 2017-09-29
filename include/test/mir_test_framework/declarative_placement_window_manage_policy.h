/*
 * Copyright © 2014-2016 Canonical Ltd.
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

#ifndef MIR_TEST_FRAMEWORK_DECLARATIVE_PLACEMENT_WINDOW_MANAGER_POLICY_H_
#define MIR_TEST_FRAMEWORK_DECLARATIVE_PLACEMENT_WINDOW_MANAGER_POLICY_H_

#include "mir_test_framework/canonical_window_manager_policy.h"
#include "mir/geometry/rectangle.h"

#include <memory>
#include <map>
#include <string>

namespace mir_test_framework
{
typedef std::map<std::string, mir::geometry::Rectangle> SurfaceGeometries;

/// DeclarativePlacementWindowManagerPolicy is a test utility server component for specifying
/// a static list of surface geometries and relative depths. Used, for example,
/// in input tests where it is necessary to set up scenarios depending on
/// multiple surfaces geometry and stacking.
class DeclarativePlacementWindowManagerPolicy : public mir_test_framework::CanonicalWindowManagerPolicy
{
public:
    DeclarativePlacementWindowManagerPolicy(
        miral::WindowManagerTools const& tools,
        SurfaceGeometries const& positions_by_name);

    virtual auto place_new_window(
        miral::ApplicationInfo const& app_info,
        miral::WindowSpecification const& request_parameters)
    -> miral::WindowSpecification override;

private:
    SurfaceGeometries const& surface_geometries_by_name;
};

}

#endif // MIR_TEST_FRAMEWORK_DECLARATIVE_PLACEMENT_WINDOW_MANAGER_POLICY_H_
