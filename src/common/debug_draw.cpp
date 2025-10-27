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

#include "mir/debug_draw.h"
#include "mir/synchronised.h"

#include <chrono>
#include <glm/vec4.hpp>

mir::Synchronised<std::vector<mir::debug::DrawCommand>> draw_commands{};

void mir::debug::draw_circle(

    mir::geometry::Point center, float radius, glm::vec4 color, std::chrono::milliseconds lifetime)
{
    draw_commands.lock()->push_back(CircleDrawCommand{center, radius, color, lifetime});
}

auto mir::debug::get_draw_commands() -> mir::Synchronised<std::vector<mir::debug::DrawCommand>> const&
{
    return draw_commands;
}
