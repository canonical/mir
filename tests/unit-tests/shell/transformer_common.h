/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_UNIT_TEST_SHELL_TRANSFORMER_COMMON_H
#define MIR_UNIT_TEST_SHELL_TRANSFORMER_COMMON_H

#include "mir/test/doubles/stub_main_loop.h"
#include "mir/test/doubles/stub_alarm.h"

#include <mir/time/clock.h>

#include <deque>

namespace mir
{
namespace test
{
namespace doubles
{
/// When this alarm is scheduled or cancelled, it will notify the caller.
/// All of other methods are provided a "stub" implementation.
class StubNotifyingAlarm : public StubAlarm
{
public:
    explicit StubNotifyingAlarm(
        std::shared_ptr<time::Clock> const& clock,
        std::function<void(StubNotifyingAlarm*, time::Timestamp)> const& on_rescheduled,
        std::function<void(StubNotifyingAlarm const*)> const& on_cancelled) :
        clock{clock},
        on_rescheduled{on_rescheduled},
        on_cancelled{on_cancelled},
        state_{cancelled}
    {
    }

    State state() const override
    {
        return state_;
    }

    ~StubNotifyingAlarm() override
    {
        on_cancelled(this);
    }

    bool reschedule_in(std::chrono::milliseconds offset) override
    {
        return reschedule_for(clock->now() + offset);
    }

    bool reschedule_for(mir::time::Timestamp timestamp) override
    {
        on_rescheduled(this, timestamp);
        state_ = pending;
        return true;
    }

    bool cancel() override
    {
        if (state_ == pending)
        {
            on_cancelled(this);
            state_ = cancelled;
        }
        return state_ == cancelled;
    }

    void about_to_be_called()
    {
        state_ = triggered;
    }

private:
    std::shared_ptr<time::Clock> const clock;
    std::function<void(StubNotifyingAlarm*, time::Timestamp)> on_rescheduled;
    std::function<void(StubNotifyingAlarm const*)> on_cancelled;
    State state_;
};

/// This MainLoop is a stub aside from the code that handles the creation of alarms.
/// When an alarm is scheduled, this class will add it to a list of functions that
/// need to be called. When an alarm is removed, this class will remove all functions
/// queued on that alarm from the list so that they are not called.
class QueuedAlarmStubMainLoop : public StubMainLoop
{
public:
    QueuedAlarmStubMainLoop(std::shared_ptr<time::Clock> const& clock) :
        clock{clock}
    {

    }

    std::unique_ptr<mir::time::Alarm> create_alarm(std::function<void()> const& f) override
    {
        return std::make_unique<StubNotifyingAlarm>(
            clock,
            [this, f = f](StubNotifyingAlarm* alarm, time::Timestamp execution_time)
            {
                pending.push_back(
                    std::make_shared<AlarmData>(
                        execution_time,
                        alarm,
                        [f=std::move(f), alarm]
                        {
                            alarm->about_to_be_called();
                            f();
                        }));
            },
            [this](StubNotifyingAlarm const* alarm)
            {
                pending.erase(
                    std::remove_if(
                        pending.begin(),
                        pending.end(),
                        [alarm](std::shared_ptr<AlarmData> const& data)
                        {
                            return data->alarm == alarm;
                        }),
                    pending.end());
            });
    }

    bool call_queued()
    {
        if (pending.empty())
            return false;

        if (pending.front()->execution_time > clock->now())
            return false;

        while (!pending.empty() && pending.front()->execution_time <= clock->now())
        {
            pending.front()->call();
            pending.pop_front();
        }

        return true;
    }

private:
    struct AlarmData
    {
        time::Timestamp const execution_time;
        StubNotifyingAlarm const* alarm;
        std::function<void()> call;

        std::strong_ordering operator<=>(AlarmData const& other)
        {
            if (execution_time < other.execution_time)
                return std::strong_ordering::less;
            else if (execution_time > other.execution_time)
                return std::strong_ordering::greater;
            else
                return std::strong_ordering::equal;
        }
    };
    std::shared_ptr<time::Clock> const clock;
    std::deque<std::shared_ptr<AlarmData>> pending;
};
}
}
}

#endif // MIR_UNIT_TEST_SHELL_TRANSFORMER_COMMON_H
