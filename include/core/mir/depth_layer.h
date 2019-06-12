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

#ifndef MIR_DEPTH_LAYER_H_
#define MIR_DEPTH_LAYER_H_

#include "mir_toolkit/common.h"

namespace mir
{

/**
* Returns the height of a MirDepthLayer
*
* As the name implies, the returned value is usable as an array index (0 is returned for the bottommost layer and there
* are no gaps between layers). The values returned for each layer are in no way stable across Mir versions, and are only
* meaningful relative to each other.
*/
auto mir_depth_layer_get_index(MirDepthLayer depth_layer) -> unsigned int;

} // namespace mir

#endif // MIR_DEPTH_LAYER_H_
