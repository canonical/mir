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

#include "basic_idle_hub.h"
#include "mir/time/alarm.h"
#include "mir/time/alarm_factory.h"
#include "mir/time/clock.h"
#include "mir/lockable_callback.h"
#include "mir/linearising_executor.h"

#include <set>

namespace ms = mir::scene;
namespace mt = mir::time;

namespace
{
/// The callback used by the alarm. Does not need to be threadsafe because it is only called into by the alarm when it
/// is fired.
class AlarmCallback: public mir::LockableCallback
{
public:
    AlarmCallback(std::mutex& mutex, std::function<void(mir::ProofOfMutexLock const& lock)> const& func)
        : mutex{mutex},
          func{func}
    {
    }

    void operator()() override
    {
        if (!lock_)
        {
            mir::fatal_error("AlarmCallback called while unlocked");
        }
        func(*lock_);
    }

    void lock() override
    {
        lock_ = std::make_unique<std::lock_guard<std::mutex>>(mutex);
    }

    void unlock() override
    {
        lock_.reset();
    }

private:
    std::mutex& mutex;
    std::function<void(mir::ProofOfMutexLock const& lock)> const func;
    std::unique_ptr<std::lock_guard<std::mutex>> lock_;
};
}

struct ms::BasicIdleHub::Multiplexer: ObserverMultiplexer<IdleStateObserver>
{
    Multiplexer()
        : ObserverMultiplexer{linearising_executor}
    {
    }

    void register_and_send_initial_state(
        std::weak_ptr<IdleStateObserver> const& observer,
        NonBlockingExecutor& executor,
        bool is_active)
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
        for_each_observer(&IdleStateObserver::idle);
    }

    void active() override
    {
        for_each_observer(&IdleStateObserver::active);
    }
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
          std::make_unique<AlarmCallback>(mutex, [this](ProofOfMutexLock const& lock)
              {
                  alarm_fired(lock);
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
    std::lock_guard<std::mutex> lock{mutex};
    poke_time = clock->now();
    schedule_alarm(lock, poke_time);
    if (!idle_multiplexers.empty())
    {
        auto const idle = std::move(idle_multiplexers);
        idle_multiplexers.clear();
        for (auto const& multiplexer : idle)
        {
            multiplexer->active();
        }
    }
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

    std::lock_guard<std::mutex> lock{mutex};
    auto const iter = timeouts.find(timeout);
    std::shared_ptr<Multiplexer> multiplexer;
    if (iter == timeouts.end())
    {
        multiplexer = std::make_shared<Multiplexer>();
        timeouts.insert({timeout, multiplexer});
        // In case this changes the first timeout
        first_timeout = timeouts.begin()->first;
        // Check if the current alarm will overshoot the new timeout (or there isn't an alarm at all)
        if (!alarm_timeout || alarm_timeout.value() > timeout)
        {
            // The alarm will not be fired before we hit our timeout
            auto const current_time = clock->now() - poke_time;
            if (current_time < timeout)
            {
                // As it stands, the alarm will be fired after we hit our timout,
                // but we have not hit it yet so the alarm needs to be moved up
                alarm_timeout = timeout;
                alarm->reschedule_in(std::chrono::duration_cast<std::chrono::milliseconds>(timeout - current_time));
            }
            else
            {
                // Our timeout has already been passed, so we are idle
                idle_multiplexers.push_back(multiplexer);
            }
        }
    }
    else
    {
        multiplexer = iter->second;
    }

    auto const is_active = (alarm_timeout && alarm_timeout.value() <= timeout);
    multiplexer->register_and_send_initial_state(observer, executor, is_active);
}

void ms::BasicIdleHub::unregister_interest(IdleStateObserver const& observer)
{
    std::lock_guard<std::mutex> lock{mutex};
    for (auto i = timeouts.begin(); i != timeouts.end();) {
        i->second->unregister_interest(observer);
        if (i->second->empty()) {
            i = timeouts.erase(i);
        } else {
            ++i;
        }
    }
    first_timeout = timeouts.empty() ? std::nullopt : std::make_optional(timeouts.begin()->first);
}

void ms::BasicIdleHub::alarm_fired(ProofOfMutexLock const& lock)
{
    if (!alarm_timeout)
    {
        // Possible if the alarm is fired but fails to get the lock until after it's been canceled
        return;
    }
    auto const iter = timeouts.find(alarm_timeout.value());
    if (iter != timeouts.end())
    {
        iter->second->idle();
        idle_multiplexers.push_back(iter->second);
    }
    schedule_alarm(lock, poke_time + alarm_timeout.value());
}

void ms::BasicIdleHub::schedule_alarm(ProofOfMutexLock const&, time::Timestamp current_time)
{
    std::optional<time::Duration> next_timeout;
    if (poke_time == current_time)
    {
        // Don't do map lookup for every poke
        next_timeout = first_timeout;
    }
    else
    {
        time::Duration const idle_time = current_time - poke_time;
        auto const iter = timeouts.upper_bound(idle_time);
        if (iter != timeouts.end())
        {
            next_timeout = iter->first;
        }
    }
    if (next_timeout)
    {
        alarm->reschedule_for(poke_time + next_timeout.value());
        alarm_timeout = next_timeout.value();
    }
    else
    {
        alarm->cancel();
        alarm_timeout = std::nullopt;
    }
}
