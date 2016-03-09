/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include "mir/test/doubles/fake_alarm_factory.h"

#include <numeric>
#include <algorithm>

namespace mtd = mir::test::doubles;

class mtd::FakeAlarmFactory::FakeAlarm : public mt::Alarm
{
public:
    FakeAlarm(
        std::function<void()> const& callback,
        std::shared_ptr<mir::time::Clock> const& clock,
        std::function<void(FakeAlarm*)> const& on_destruction);
    ~FakeAlarm() override;

    void time_updated();
    int wakeup_count() const;

    bool cancel() override;
    State state() const override;

    bool reschedule_in(std::chrono::milliseconds delay) override;
    bool reschedule_for(mir::time::Timestamp timeout) override;

private:
    int triggered_count;
    std::function<void()> const callback;
    State alarm_state;
    mir::time::Timestamp triggers_at;
    std::shared_ptr<mt::Clock> clock;
    std::function<void(FakeAlarm*)> const on_destruction;
};


mtd::FakeAlarmFactory::FakeAlarm::FakeAlarm(
    std::function<void()> const& callback,
    std::shared_ptr<mir::time::Clock> const& clock,
    std::function<void(FakeAlarm*)> const& on_destruction)
    : triggered_count{0},
      callback{callback},
      alarm_state{State::cancelled},
      triggers_at{mir::time::Timestamp::max()},
      clock{clock},
      on_destruction{on_destruction}
{
}

mtd::FakeAlarmFactory::FakeAlarm::~FakeAlarm()
{
    on_destruction(this);
}

void mtd::FakeAlarmFactory::FakeAlarm::time_updated()
{
    if (clock->now() > triggers_at)
    {
        triggers_at = mir::time::Timestamp::max();
        alarm_state = State::triggered;
        callback();
        ++triggered_count;
    }
}

int mtd::FakeAlarmFactory::FakeAlarm::wakeup_count() const
{
    return triggered_count;
}

bool mtd::FakeAlarmFactory::FakeAlarm::cancel()
{
    if (alarm_state == State::pending)
    {
        triggers_at = mir::time::Timestamp::max();
        alarm_state = State::cancelled;
        return true;
    }
    return false;
}

mt::Alarm::State mtd::FakeAlarmFactory::FakeAlarm::state() const
{
    return alarm_state;
}

bool mtd::FakeAlarmFactory::FakeAlarm::reschedule_in(std::chrono::milliseconds delay)
{
    bool rescheduled = alarm_state == State::pending;
    alarm_state = State::pending;
    triggers_at = clock->now() + std::chrono::duration_cast<mt::Duration >(delay);
    return rescheduled;
}

bool mtd::FakeAlarmFactory::FakeAlarm::reschedule_for(mir::time::Timestamp timeout)
{
    bool rescheduled = alarm_state == State::pending;
    if (timeout > clock->now())
    {
        alarm_state = State::pending;
        triggers_at = timeout;
    }
    else
    {
        callback();
        triggers_at = mir::time::Timestamp::max();
        alarm_state = State::triggered;
    }
    return rescheduled;
}

mtd::FakeAlarmFactory::FakeAlarmFactory()
    : clock{std::make_shared<mtd::AdvanceableClock>()}
{
}

std::unique_ptr<mt::Alarm> mtd::FakeAlarmFactory::create_alarm(
    std::function<void()> const& callback)
{
    std::unique_ptr<mt::Alarm> alarm = std::make_unique<FakeAlarm>(
        callback,
        clock,
        [this](FakeAlarm* destroying)
        {
            alarms.erase(std::remove(alarms.begin(), alarms.end(), destroying), alarms.end());
        });
    alarms.push_back(static_cast<FakeAlarm*>(alarm.get()));
    return alarm;
}

std::unique_ptr<mt::Alarm> mtd::FakeAlarmFactory::create_alarm(
    std::shared_ptr<mir::LockableCallback> const& /*callback*/)
{
    throw std::logic_error{"Lockable alarm creation not implemented for fake alarms"};
}

void mtd::FakeAlarmFactory::advance_by(mt::Duration step)
{
    clock->advance_by(step);
    // Guard against alarms deleting themselves from their callback...
    auto temp_alarms = alarms;
    for (auto& alarm : temp_alarms)
    {
        alarm->time_updated();
    }
}

void mtd::FakeAlarmFactory::advance_smoothly_by(mt::Duration step)
{
    using namespace std::literals::chrono_literals;
    auto const step_by = 1ms;
    while (step.count() > 0)
    {
        advance_by(step_by);
        step -= step_by;
    }
}

int mtd::FakeAlarmFactory::wakeup_count() const
{
    return std::accumulate(
        alarms.begin(),
        alarms.end(),
        0,
        [](int count, FakeAlarm const* alarm)
        {
            return count + alarm->wakeup_count();
        });
}
