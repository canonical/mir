/*
 * Copyright © Canonical Ltd.
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
 */

#include "mir/depth_layer.h"

// GCC and Clang both ensure the switch is exhaustive.
// GCC, however, gets a "control reaches end of non-void function" warning without this
#ifndef __clang__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
#endif

auto mir::mir_depth_layer_get_index(MirDepthLayer const depth_layer) -> unsigned int
{
    switch (depth_layer)
    {
    case mir_depth_layer_background:
    case mir_depth_layer_below:
    case mir_depth_layer_application:
    case mir_depth_layer_always_on_top:
    case mir_depth_layer_above:
    case mir_depth_layer_overlay:
        return depth_layer;
    }
}

#ifndef __clang__
#pragma GCC diagnostic pop
#endif
