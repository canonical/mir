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

#include <memory>

namespace
{
std::weak_ptr<mir::debug::DebugDrawHandler> debug_draw_handler;
}

void mir::debug::init(std::shared_ptr<mir::debug::DebugDrawHandler> const& handler)
{
    debug_draw_handler = handler;
}

void mir::debug::draw_circle(mir::debug::CircleDrawCommand&& command)
{
    if (auto handler = debug_draw_handler.lock())
    {
        handler->add(std::move(command));
    }
}

void mir::debug::draw_line(mir::debug::LineDrawCommand&& command)
{
    if (auto handler = debug_draw_handler.lock())
    {
        handler->add(std::move(command));
    }
}