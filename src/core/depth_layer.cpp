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

#include <mir/depth_layer.h>
#include <mir/fatal.h>

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
    MIR_FATAL_ERROR("Invalid MirDepthLayer value: {}", static_cast<int>(depth_layer));
}
