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
#include "mir/time/types.h"
#include "mir/proof_of_mutex_lock.h"

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

class BasicIdleHub : public IdleHub
{
public:
    BasicIdleHub(
        std::shared_ptr<time::Clock> const& clock,
        time::AlarmFactory& alarm_factory);

    ~BasicIdleHub();

    void poke() override;

    void register_interest(
        std::weak_ptr<IdleStateObserver> const& observer,
        std::chrono::milliseconds timeout) override;

    void register_interest(
        std::weak_ptr<IdleStateObserver> const& observer,
        Executor& executor,
        std::chrono::milliseconds timeout) override;

    void unregister_interest(IdleStateObserver const& observer) override;

private:
    struct Multiplexer;

    void alarm_fired();
    void schedule_alarm(ProofOfMutexLock const& lock, time::Timestamp current_time);

    std::shared_ptr<time::Clock> const clock;
    std::unique_ptr<time::Alarm> const alarm;
    std::mutex mutex;
    /// Maps timeouts (times from last poke) to the multiplexers that need to be fired at those times.
    std::map<std::chrono::milliseconds, std::shared_ptr<Multiplexer>> timeouts;
    std::optional<std::chrono::milliseconds> first_timeout;
    std::vector<std::shared_ptr<Multiplexer>> idle_multiplexers;
    /// The timestamp when we were last poked
    time::Timestamp poke_time;
    /// Amount of time after the poke time before the alarm fires
    std::chrono::milliseconds alarm_timeout{0};
};
}
}

#endif // MIR_SCENE_BASIC_IDLE_HUB_H_
