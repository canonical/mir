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

#ifndef MIR_SCENE_BASIC_IDLE_HUB_H_
#define MIR_SCENE_BASIC_IDLE_HUB_H_

#include <mir/scene/idle_hub.h>
#include <mir/observer_multiplexer.h>
#include <mir/time/types.h>
#include <mir/synchronised.h>

#include <mutex>
#include <map>

namespace mir
{
namespace time
{
class Clock;
class Alarm;
class AlarmFactory;
}
namespace scene
{
/// Users can register an IdleStateObserver to be notified after a given timeout using the IdleHub interface. This class
/// keeps track of all registered observers and organizes them by timeout. It sets an alarm for the next timeout, and
/// when the alarm fires it notifies the observer it is is now idle. When this class gets poked (generally by an input
/// event), Mir is no longer considered to be idle and any idle observers get notified. After each poke the alarm gets
/// rescheduled based on the first timeout.
class BasicIdleHub : public IdleHub, public std::enable_shared_from_this<BasicIdleHub>
{
public:
    BasicIdleHub(
        std::shared_ptr<time::Clock> const& clock,
        time::AlarmFactory& alarm_factory);

    ~BasicIdleHub();

    void poke() override;

    void register_interest(
        std::weak_ptr<IdleStateObserver> const& observer,
        time::Duration timeout) override;

    void register_interest(
        std::weak_ptr<IdleStateObserver> const& observer,
        NonBlockingExecutor& executor,
        time::Duration timeout) override;

    void unregister_interest(IdleStateObserver const& observer) override;

    auto inhibit_idle() -> std::shared_ptr<IdleHub::WakeLock> override;

private:
    class AlarmCallback;
    struct Multiplexer;
    struct PendingRegistration;

    struct State
    {
        std::weak_ptr<IdleHub::WakeLock> wake_lock;
        /// Maps timeouts (times from last poke) to the multiplexers that need to be fired at those times.
        std::map<time::Duration, std::shared_ptr<Multiplexer>> timeouts;
        /// Should always be equal to timeouts.begin()->first, or nullopt if timeouts is empty. Only purpose is so we don't
        /// need to do a map lookup on every poke (we are poked for every input event).
        std::optional<time::Duration> first_timeout;
        std::vector<std::shared_ptr<Multiplexer>> idle_multiplexers;
        /// The timestamp when we were last poked
        time::Timestamp poke_time;
        /// Amount of time after the poke time before the alarm fires, or none if the alarm is not scheduled
        std::optional<time::Duration> alarm_timeout;
    };

    void poke_locked(State& state);
    void alarm_fired(State& state);
    void schedule_alarm(State& state, time::Timestamp current_time);

    std::shared_ptr<time::Clock> const clock;
    std::unique_ptr<time::Alarm> const alarm;
    mir::Synchronised<State> synchronised_state;
};
}
}

#endif // MIR_SCENE_BASIC_IDLE_HUB_H_
