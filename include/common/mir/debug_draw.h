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

#include "mir/synchronised.h"
#include <mir/geometry/point.h>

#include <chrono>
#include <variant>

#include <glm/vec4.hpp>

namespace mir
{
class Server;
namespace compositor
{
class SceneElement;
}
namespace debug
{
// TODO handling multiple monitors?

struct CircleDrawCommand
{
    mir::geometry::Point center;
    float radius{50};
    glm::vec4 color{1.0f, 0.0f, 1.0f, 1.0f}; // TODO does alpha matter here?
    std::chrono::milliseconds lifetime{1000};
};

struct LineDrawCommand
{
    mir::geometry::Point start;
    mir::geometry::Point end;
    float thickness{10.0f};
    glm::vec4 color{1.0f, 0.0f, 1.0f, 1.0f};
    std::chrono::milliseconds lifetime{1000};
};

using DrawCommand = std::variant<CircleDrawCommand, LineDrawCommand>;

// TODO: Required parameters + Options struct?
void draw_circle(CircleDrawCommand&&);

void draw_line(LineDrawCommand&&);

auto get_draw_commands() -> mir::Synchronised<std::vector<DrawCommand>>&;
}
}

#endif
