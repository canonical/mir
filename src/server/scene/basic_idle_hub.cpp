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

namespace ms = mir::scene;
namespace mt = mir::time;

namespace
{
struct DirectExecutor: mir::Executor
{
    void spawn(std::function<void()>&& work) override
    {
        work();
    }
} direct_executor;
}

class ms::BasicIdleHub::Multiplexer: public ObserverMultiplexer<IdleStateObserver>
{
public:
    Multiplexer()
        : ObserverMultiplexer{direct_executor}
    {
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

ms::BasicIdleHub::BasicIdleHub(
    std::shared_ptr<time::Clock> const& clock,
    mt::AlarmFactory& alarm_factory)
    : clock{clock},
      alarm{alarm_factory.create_alarm([this]()
          {
              alarm_fired();
          })},
      poke_time{clock->now()}
{
}

ms::BasicIdleHub::~BasicIdleHub()
{
    alarm->cancel();
}

void ms::BasicIdleHub::poke()
{
    std::unique_lock<std::mutex> lock{mutex};
    poke_time = clock->now();
    schedule_alarm(lock, poke_time);
    if (!idle_multiplexers.empty())
    {
        auto const idle = std::move(idle_multiplexers);
        idle_multiplexers.clear();
        lock.unlock();
        for (auto const& multiplexer : idle)
        {
            multiplexer->active();
        }
    }
}

void ms::BasicIdleHub::register_interest(
        std::weak_ptr<IdleStateObserver> const& observer,
        std::chrono::milliseconds timeout)
{
    register_interest(observer, direct_executor, timeout);
}

void ms::BasicIdleHub::register_interest(
    std::weak_ptr<IdleStateObserver> const& observer,
    Executor& executor,
    std::chrono::milliseconds timeout)
{
    std::unique_lock<std::mutex> lock{mutex};
    auto const iter = timeouts.find(timeout);
    bool is_active = true;
    if (iter == timeouts.end())
    {
        auto const multiplexer = std::make_shared<Multiplexer>();
        multiplexer->register_interest(observer, executor);
        timeouts.insert({timeout, multiplexer});
        first_timeout = timeouts.begin()->first;
        auto const current_time = clock->now() - poke_time;
        if (alarm_timeout <= timeout)
        {
            // The alarm will be fired before we hit our timeout
            is_active = true;
        }
        else if (current_time < timeout)
        {
            // The alarm will be fired after we hit our timout,
            // but we have not hit it yet so the alarm needs to be moved up
            is_active = true;
            alarm_timeout = timeout;
            alarm->reschedule_in(std::chrono::duration_cast<std::chrono::milliseconds>(timeout - current_time));
        }
        else
        {
            // This timeout has already been passed
            is_active = false;
            idle_multiplexers.push_back(multiplexer);
        }
    }
    else
    {
        iter->second->register_interest(observer, executor);
        is_active = (alarm_timeout <= timeout);
    }
    lock.unlock();
    if (auto const shared = observer.lock())
    {
        if (is_active)
        {
            shared->active();
        }
        else
        {
            shared->idle();
        }
    }
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
    first_timeout = timeouts.begin()->first;
}

void ms::BasicIdleHub::alarm_fired()
{
    std::unique_lock<std::mutex> lock{mutex};
    auto const iter = timeouts.find(alarm_timeout);
    std::shared_ptr<Multiplexer> multiplexer;
    if (iter != timeouts.end())
    {
        multiplexer = iter->second;
        idle_multiplexers.push_back(iter->second);
    }
    schedule_alarm(lock, poke_time + alarm_timeout);
    lock.unlock();
    if (multiplexer)
    {
        multiplexer->idle();
    }
}

void ms::BasicIdleHub::schedule_alarm(ProofOfMutexLock const&, time::Timestamp current_time)
{
    std::optional<std::chrono::milliseconds> next_timeout;
    if (poke_time == current_time)
    {
        // Don't do map lookup for every poke
        next_timeout = first_timeout;
    }
    else
    {
        auto const idle_time = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - poke_time);
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
        alarm_timeout = std::chrono::milliseconds{0};
    }
}
