/*
 * Copyright © 2019 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: William Wold <william.wold@canonical.com>
 */

#include "mir/depth_layer.h"

#include <stdexcept>

auto mir::mir_depth_layer_get_index(MirDepthLayer depth_layer) -> unsigned int
{
    switch (depth_layer)
    {
    case mir_depth_layer_background:        return 0;
    case mir_depth_layer_below:             return 1;
    case mir_depth_layer_application:       return 2;
    case mir_depth_layer_always_on_top:     return 3;
    case mir_depth_layer_above:             return 4;
    case mir_depth_layer_overlay:           return 5;
    }
    // GCC and Clang both ensure the switch is exhaustive.
    // GCC, however, gets a "control reaches end of non-void function" warning without this
#ifndef __clang__
    throw std::logic_error("Invalid MirDepthLayer in mir::mir_depth_layer_get_index()");
#endif
}
