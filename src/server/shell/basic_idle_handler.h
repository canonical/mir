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
 */

#ifndef MIR_SHELL_BASIC_IDLE_HANDLER_H_
#define MIR_SHELL_BASIC_IDLE_HANDLER_H_

#include "mir/shell/idle_handler.h"
#include "mir/proof_of_mutex_lock.h"

#include <memory>
#include <vector>

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
}
namespace shell
{
class DisplayConfigurationController;

class BasicIdleHandler : public IdleHandler
{
public:
    BasicIdleHandler(
        std::shared_ptr<scene::IdleHub> const& idle_hub,
        std::shared_ptr<input::Scene> const& input_scene,
        std::shared_ptr<graphics::GraphicBufferAllocator> const& allocator,
        std::shared_ptr<shell::DisplayConfigurationController> const& display_config_controller);

    ~BasicIdleHandler();

    void set_display_off_timeout(std::optional<time::Duration> timeout) override;

private:
    void clear_observers(ProofOfMutexLock const&);

    std::shared_ptr<scene::IdleHub> const idle_hub;
    std::shared_ptr<input::Scene> const input_scene;
    std::shared_ptr<graphics::GraphicBufferAllocator> const allocator;
    std::shared_ptr<shell::DisplayConfigurationController> const display_config_controller;

    std::mutex mutex;
    std::optional<time::Duration> current_off_timeout;
    std::vector<std::shared_ptr<scene::IdleStateObserver>> observers;
};

}
}

#endif // MIR_SHELL_BASIC_IDLE_HANDLER_H_
