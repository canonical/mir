/*
 * Copyright © 2021 Canonical Ltd.
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
 *
 * Authored By: William Wold <william.wold@canonical.com>
 */

#ifndef MIR_SCENE_BASIC_IDLE_HUB_H_
#define MIR_SCENE_BASIC_IDLE_HUB_H_

#include "mir/scene/idle_hub.h"
#include "mir/observer_multiplexer.h"

#include <mutex>

namespace mir
{
namespace time
{
class Alarm;
class AlarmFactory;
}
namespace scene
{

class IdleStateMultiplexer: mir::Executor, public ObserverMultiplexer<IdleStateObserver>
{
public:
    IdleStateMultiplexer()
        : ObserverMultiplexer{static_cast<Executor&>(*this)}
    {
    }

    // By default, dispatch events immediately. The user can override this behavior by setting their own executor.
    void spawn(std::function<void()>&& work) override
    {
        work();
    }

    void idle_state_changed(IdleState state) override
    {
        for_each_observer(&IdleStateObserver::idle_state_changed, state);
    }
};

class BasicIdleHub : public IdleHub
{
public:
    struct StateEntry
    {
        std::chrono::milliseconds time_from_previous;
        IdleState state;
    };

    /// IdleState::awake is always the initial state. If state_sequence is empty, it is the only state.
    BasicIdleHub(
        std::vector<StateEntry> const& state_sequence,
        std::shared_ptr<time::AlarmFactory> const& alarm_factory);

    ~BasicIdleHub();

    auto state() -> IdleState override;
    void poke() override;

    /// Implement ObserverRegistrar<IdleStateObserver>
    /// @{
    void register_interest(std::weak_ptr<IdleStateObserver> const& observer) override
    {
        multiplexer.register_interest(observer);
        send_initial_state(observer);
    }
    void register_interest(std::weak_ptr<IdleStateObserver> const& observer, Executor& executor) override
    {
        multiplexer.register_interest(observer, executor);
        send_initial_state(observer);
    }
    void unregister_interest(IdleStateObserver const& observer) override
    {
        multiplexer.unregister_interest(observer);
    }
    /// @}

private:
    void increment_state();
    /// May unlock the lock
    void set_state_index(std::unique_lock<std::mutex>& lock, int index);
    void send_initial_state(std::weak_ptr<IdleStateObserver> const& observer);

    IdleStateMultiplexer multiplexer;
    std::vector<StateEntry> const state_sequence;
    std::unique_ptr<time::Alarm> const next_state_alarm;
    std::mutex mutex;
    int current_state_index;
};
}
}

#endif // MIR_SCENE_BASIC_IDLE_HUB_H_
