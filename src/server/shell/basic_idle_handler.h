/*
 * Copyright © Canonical Ltd.
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
#include "mir/executor.h"
#include "mir/observer_multiplexer.h"

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
class SessionLock;
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
        std::shared_ptr<shell::DisplayConfigurationController> const& display_config_controller,
        std::shared_ptr<scene::SessionLock> const& session_lock);

    ~BasicIdleHandler();

    void set_display_off_timeout(std::optional<time::Duration> timeout) override;

    void set_display_off_timeout_on_lock(std::optional<time::Duration> timeout) override;

    void register_interest(std::weak_ptr<IdleHandlerObserver> const&) override;

    void register_interest(
        std::weak_ptr<IdleHandlerObserver> const&,
        Executor&) override;

    void register_early_observer(
        std::weak_ptr<IdleHandlerObserver> const&,
        Executor&) override;

    void unregister_interest(IdleHandlerObserver const&) override;

private:
    class SessionLockListener;

    void register_observers(ProofOfMutexLock const&);
    void clear_observers(ProofOfMutexLock const&);

    std::shared_ptr<scene::IdleHub> const idle_hub;
    std::shared_ptr<input::Scene> const input_scene;
    std::shared_ptr<graphics::GraphicBufferAllocator> const allocator;
    std::shared_ptr<shell::DisplayConfigurationController> const display_config_controller;
    std::shared_ptr<scene::SessionLock> const session_lock;
    std::shared_ptr<SessionLockListener> const session_lock_monitor;

    std::mutex mutex;
    std::optional<time::Duration> current_off_timeout;
    std::optional<time::Duration> current_off_timeout_on_lock;
    std::vector<std::shared_ptr<scene::IdleStateObserver>> observers;

    class BasicIdleHandlerObserverMultiplexer: public ObserverMultiplexer<IdleHandlerObserver>
    {
    public:
        BasicIdleHandlerObserverMultiplexer()
            : ObserverMultiplexer{immediate_executor}
        {
        }

        void dim() override
        {
            for_each_observer(&IdleHandlerObserver::dim);
        }

        void off() override
        {
            for_each_observer(&IdleHandlerObserver::off);
        }

        void wake() override
        {
            for_each_observer(&IdleHandlerObserver::wake);
        }
    };
    BasicIdleHandlerObserverMultiplexer multiplexer;
};

}
}

#endif // MIR_SHELL_BASIC_IDLE_HANDLER_H_
