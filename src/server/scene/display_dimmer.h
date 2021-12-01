/*
 * Copyright © 2021 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: William Wold <william.wold@canonical.com>
 */

#ifndef MIR_SCENE_DISPLAY_DIMMER_H_
#define MIR_SCENE_DISPLAY_DIMMER_H_

#include <memory>

namespace mir
{
namespace graphics
{
class GraphicBufferAllocator;
}
namespace input
{
class Scene;
}
namespace scene
{
class IdleHub;
class IdleStateObserver;

class DisplayDimmer
{
public:
    DisplayDimmer(
        std::shared_ptr<IdleHub> const& idle_hub,
        std::shared_ptr<input::Scene> const& input_scene,
        std::shared_ptr<graphics::GraphicBufferAllocator> const& allocator);

    ~DisplayDimmer() = default;

private:
    std::shared_ptr<IdleHub> const idle_hub;
    std::shared_ptr<IdleStateObserver> const idle_state_observer;
};

}
}

#endif // MIR_SCENE_DISPLAY_DIMMER_H_
