/*
 * Copyright © Canonical Ltd.
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
 */

#ifndef MIRAL_IS_OCCLUDED_H
#define MIRAL_IS_OCCLUDED_H

#include "mir/geometry/forward.h"

#include <vector>

namespace miral
{
auto is_occluded(
    mir::geometry::Rectangle test_rectangle,
    std::vector<mir::geometry::Rectangle> const& occluding_rectangles,
    mir::geometry::Size min_visible_size) -> bool;
}

#endif
