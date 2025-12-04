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

#include "basic_idle_hub.h"
#include <mir/time/alarm.h>
#include <mir/time/alarm_factory.h>
#include <mir/time/clock.h>
#include <mir/lockable_callback.h>
#include <mir/executor.h>
#include <mir/fatal.h>

#include <set>

namespace ms = mir::scene;
namespace mt = mir::time;

/// The callback used by the alarm. Does not need to be threadsafe because it is only called into by the alarm when it
/// is fired.
class ms::BasicIdleHub::AlarmCallback: public mir::LockableCallback
{
public:
    AlarmCallback(Synchronised<State>& synchronised_state, std::function<void(State& state)> const& func)
        : synchronised_state{synchronised_state},
          func{func}
    {
    }

    void operator()() override
    {
        if (!locked)
        {
            mir::fatal_error("AlarmCallback called while unlocked");
        }
        func(*locked.value());
    }

    void lock() override
    {
        locked.emplace(synchronised_state.lock());
    }

    void unlock() override
    {
        locked.reset();
    }

private:
    Synchronised<State>& synchronised_state;
    std::function<void(State& state)> const func;
    std::optional<Synchronised<State>::Locked> locked;
};

struct ms::BasicIdleHub::Multiplexer: ObserverMultiplexer<IdleStateObserver>
{
    Multiplexer()
        : ObserverMultiplexer{linearising_executor}
    {
    }

    void register_and_send_initial_state(
        std::weak_ptr<IdleStateObserver> const& observer,
        NonBlockingExecutor& executor)
    {
        register_interest(observer, executor);
        if (auto const locked = observer.lock())
        {
            if (is_active)
            {
                for_single_observer(*locked, &IdleStateObserver::active);
            }
            else
            {
                for_single_observer(*locked, &IdleStateObserver::idle);
            }
        }
    }

    void idle() override
    {
        if (is_active)
        {
            is_active = false;
            for_each_observer(&IdleStateObserver::idle);
        }
    }

    void active() override
    {
        if (!is_active)
        {
            is_active = true;
            for_each_observer(&IdleStateObserver::active);
        }
    }

private:
    bool is_active{true};
};

struct ms::BasicIdleHub::PendingRegistration
{
    PendingRegistration()
    {
    }

    std::recursive_mutex mutex;
    std::set<IdleStateObserver const*> observers;
};

ms::BasicIdleHub::BasicIdleHub(
    std::shared_ptr<time::Clock> const& clock,
    mt::AlarmFactory& alarm_factory)
    : clock{clock},
      alarm{alarm_factory.create_alarm(
          std::make_unique<AlarmCallback>(synchronised_state, [this](State& state)
              {
                  alarm_fired(state);
              }))}
{
    poke();
}

ms::BasicIdleHub::~BasicIdleHub()
{
    alarm->cancel();
}

void ms::BasicIdleHub::poke()
{
    poke_locked(*synchronised_state.lock());
}

void ms::BasicIdleHub::register_interest(
    std::weak_ptr<IdleStateObserver> const& observer,
    time::Duration timeout)
{
    register_interest(observer, linearising_executor, timeout);
}

void ms::BasicIdleHub::register_interest(
    std::weak_ptr<IdleStateObserver> const& observer,
    NonBlockingExecutor& executor,
    time::Duration timeout)
{
    auto const shared_observer = observer.lock();
    if (!shared_observer)
    {
        return;
    }

    auto state = synchronised_state.lock();
    auto const iter = state->timeouts.find(timeout);
    std::shared_ptr<Multiplexer> multiplexer;
    if (iter == state->timeouts.end())
    {
        multiplexer = std::make_shared<Multiplexer>();
        state->timeouts.insert({timeout, multiplexer});
        // In case this changes the first timeout
        state->first_timeout = state->timeouts.begin()->first;
        // Check if the current alarm will overshoot the new timeout (or there isn't an alarm at all)
        if (state->wake_lock.expired() && (!state->alarm_timeout || state->alarm_timeout.value() > timeout))
        {
            // The alarm will not be fired before we hit our timeout
            auto const current_time = clock->now() - state->poke_time;
            if (current_time < timeout)
            {
                // As it stands, the alarm will be fired after we hit our timout,
                // but we have not hit it yet so the alarm needs to be moved up
                state->alarm_timeout = timeout;
                alarm->reschedule_in(std::chrono::duration_cast<std::chrono::milliseconds>(timeout - current_time));
            }
            else
            {
                // Our timeout has already been passed, so we are idle
                multiplexer->idle();
                state->idle_multiplexers.push_back(multiplexer);
            }
        }
    }
    else
    {
        multiplexer = iter->second;
    }

    multiplexer->register_and_send_initial_state(observer, executor);
}

void ms::BasicIdleHub::unregister_interest(IdleStateObserver const& observer)
{
    auto state = synchronised_state.lock();
    for (auto i = state->timeouts.begin(); i != state->timeouts.end();) {
        i->second->unregister_interest(observer);
        if (i->second->empty()) {
            i = state->timeouts.erase(i);
        } else {
            ++i;
        }
    }
    state->first_timeout = state->timeouts.empty() ?
        std::nullopt :
        std::make_optional(state->timeouts.begin()->first);
}

void ms::BasicIdleHub::poke_locked(State& state)
{
    if (!state.wake_lock.expired())
    {
        return;
    }

    state.poke_time = clock->now();
    schedule_alarm(state, state.poke_time);
    if (!state.idle_multiplexers.empty())
    {
        auto const idle = std::move(state.idle_multiplexers);
        state.idle_multiplexers.clear();
        for (auto const& multiplexer : idle)
        {
            multiplexer->active();
        }
    }
}

void ms::BasicIdleHub::alarm_fired(State& state)
{
    if (!state.alarm_timeout)
    {
        // Possible if the alarm is fired but fails to get the lock until after it's been canceled
        return;
    }
    auto const iter = state.timeouts.find(state.alarm_timeout.value());
    if (iter != state.timeouts.end())
    {
        iter->second->idle();
        state.idle_multiplexers.push_back(iter->second);
    }
    schedule_alarm(state, state.poke_time + state.alarm_timeout.value());
}

void ms::BasicIdleHub::schedule_alarm(State& state, time::Timestamp current_time)
{
    std::optional<time::Duration> next_timeout;
    if (state.poke_time == current_time)
    {
        // Don't do map lookup for every poke
        next_timeout = state.first_timeout;
    }
    else
    {
        time::Duration const idle_time = current_time - state.poke_time;
        auto const iter = state.timeouts.upper_bound(idle_time);
        if (iter != state.timeouts.end())
        {
            next_timeout = iter->first;
        }
    }
    if (next_timeout)
    {
        alarm->reschedule_for(state.poke_time + next_timeout.value());
        state.alarm_timeout = next_timeout.value();
    }
    else
    {
        alarm->cancel();
        state.alarm_timeout = std::nullopt;
    }
}

struct ms::IdleHub::WakeLock
{
    WakeLock(std::weak_ptr<IdleHub> idle_hub) : idle_hub{std::move(idle_hub)}
    {
    }

    ~WakeLock()
    {
        if (auto const shared_hub = idle_hub.lock())
        {
            shared_hub->poke();
        }
    }

private:
    std::weak_ptr<IdleHub> const idle_hub;
};

auto ms::BasicIdleHub::inhibit_idle() -> std::shared_ptr<WakeLock>
{
    auto state = synchronised_state.lock();
    if (auto const shared_wake_lock = state->wake_lock.lock()) // wake_lock is already held
    {
        return shared_wake_lock;
    }
    else // wake_lock is not being held
    {
        poke_locked(*state);
        auto result = std::make_shared<WakeLock>(shared_from_this());
        alarm->cancel();
        state->wake_lock = result;
        return result;
    }
}
