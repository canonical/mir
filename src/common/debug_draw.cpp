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

#include <mir/debug_draw.h>
#include <mir/geometry/point.h>

#include <chrono>
#include <glm/vec4.hpp>
#include <mutex>

struct Circle
{
    mir::geometry::Point center;
    float radius;
    glm::vec4 color;
    float thickness;
    std::chrono::milliseconds lifetime;
};

namespace
{
    std::mutex mutex;
    std::vector<Circle> circle_drawlist;
}

void mir::debug::draw_circle(

    mir::geometry::Point center, float radius, glm::vec4 color, float thickness, std::chrono::milliseconds lifetime)
{
    // Add circle to drawlist
    // Set alarm to remove from drawlist
    std::unique_lock lock{mutex};
    circle_drawlist.push_back({center, radius, color, thickness, lifetime});

    // Need to add this to the very top layer of the scene during compositing/drawing
    // Maybe in `SurfaceStack::scene_elements_for` or after it
    //
    // When drawing, decrement the lifetime by the frametime (if available),
    // and don't draw if < 0
}
