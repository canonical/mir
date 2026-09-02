/*
 * Copyright © Canonical Ltd.
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
 */

#ifndef MIRAL_MAGNIFIER_CONTROLS_H
#define MIRAL_MAGNIFIER_CONTROLS_H

#include "magnifier_layout.h"

#include <mir/geometry/point.h>
#include <mir/geometry/size.h>

namespace miral::magnifier_controls
{
constexpr int handle_diameter{48};
constexpr int zoom_stack_padding{8};
constexpr int visual_edge_clearance{handle_diameter};
constexpr mir::geometry::Size minimum_visual_size{
    2 * handle_diameter,
    3 * handle_diameter + zoom_stack_padding};

enum class HandleKind
{
    drag,
    resize,
    zoom_in,
    zoom_out
};

struct Positions
{
    mir::geometry::Point drag;
    mir::geometry::Point resize;
    mir::geometry::Point zoom_in;
    mir::geometry::Point zoom_out;

    auto for_kind(HandleKind kind) const -> mir::geometry::Point;

    bool operator==(Positions const&) const = default;
};

auto positions_for(
    miral::magnifier_layout::FreePlacement const& placement,
    double magnification) -> Positions;

auto contains_content(
    miral::magnifier_layout::FreePlacement const& placement,
    double magnification,
    mir::geometry::PointD const& point) -> bool;
}

#endif
