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

#ifndef MIR_DEBUG_DRAW_H_
#define MIR_DEBUG_DRAW_H_

#include <mir/geometry/forward.h>

#include <chrono>
#include <glm/fwd.hpp>

namespace mir
{
namespace debug
{
void draw_circle(
    mir::geometry::Point center, float radius, glm::vec4 color, float thickness, std::chrono::milliseconds lifetime);
}
}

#endif
