/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_COMPOSITOR_DEBUG_DRAW_MANAGER_H_
#define MIR_COMPOSITOR_DEBUG_DRAW_MANAGER_H_

#include "mir/debug_draw.h"
#include "mir/synchronised.h"

#include <vector>
#include <memory>
#include <chrono>

namespace mir
{
namespace graphics
{
class GraphicBufferAllocator;
}
namespace time
{
class Clock;
}
namespace compositor
{
class SceneElement;

class DebugDrawManager : public mir::debug::DebugDrawHandler
{
public:
    DebugDrawManager(
        std::shared_ptr<graphics::GraphicBufferAllocator> const& allocator,
        std::shared_ptr<time::Clock> const& clock);
    ~DebugDrawManager() = default;

    DebugDrawManager(DebugDrawManager const&) = delete;
    DebugDrawManager& operator=(DebugDrawManager const&) = delete;

    void add(mir::debug::DrawCommand&& command) override;

    auto process_commands() -> std::vector<std::shared_ptr<SceneElement>>;

private:
    std::shared_ptr<graphics::GraphicBufferAllocator> const allocator;
    std::shared_ptr<time::Clock> const clock;
    std::chrono::steady_clock::time_point last_render_time;
    mir::Synchronised<std::vector<mir::debug::DrawCommand>> commands;
};

}
}

#endif // MIR_COMPOSITOR_DEBUG_DRAW_MANAGER_H_