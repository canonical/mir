/*
 * Copyright © 2023 Canonical Ltd.
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

#ifndef MIR_COMMON_EVENT_HELPERS_H_
#define MIR_COMMON_EVENT_HELPERS_H_

#include "event.h"

#include <functional>

namespace mir
{
namespace events
{
/// A function that takes and returns a global and optional local position
using MapPositionFunc = std::function<
    std::pair<geometry::PointF, std::optional<geometry::PointF>>(
        geometry::PointF global,
        std::optional<geometry::PointF> local)>;

/// Calls the given function for all positions in the event (could be multiple in the case of touch events) and updates
/// the local and global positions based on the returned values.
void map_positions(MirEvent& event, MapPositionFunc const& func);
}
}

#endif // MIR_COMMON_EVENT_HELPERS_H_
