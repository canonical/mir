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

#include <list>

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
        std::function<void(StubNotifyingAlarm const*)> const& on_rescheduled,
        std::function<void(StubNotifyingAlarm const*)> const& on_cancelled) :
        on_rescheduled{on_rescheduled},
        on_cancelled{on_cancelled}
    {
    }

    ~StubNotifyingAlarm() override
    {
        on_cancelled(this);
    }

    bool reschedule_in(std::chrono::milliseconds) override
    {
        on_rescheduled(this);
        return true;
    }

    bool reschedule_for(mir::time::Timestamp) override
    {
        on_rescheduled(this);
        return true;
    }

private:
    std::function<void(StubNotifyingAlarm const*)> on_rescheduled;
    std::function<void(StubNotifyingAlarm const*)> on_cancelled;
};

/// This MainLoop is a stub aside from the code that handles the creation of alarms.
/// When an alarm is scheduled, this class will add it to a list of functions that
/// need to be called. When an alarm is removed, this class will remove all functions
/// queued on that alarm from the list so that they are not called.
class QueuedAlarmStubMainLoop : public StubMainLoop
{
public:
    std::unique_ptr<mir::time::Alarm> create_alarm(std::function<void()> const& f) override
    {
        return std::make_unique<StubNotifyingAlarm>(
            [this, f=f](StubNotifyingAlarm const* alarm)
            {
                pending.push_back(std::make_shared<AlarmData>(alarm, f));
            },
            [this](StubNotifyingAlarm const* alarm)
            {
                pending.erase(std::remove_if(pending.begin(), pending.end(), [alarm](std::shared_ptr<AlarmData> const& data)
                {
                    return data->alarm == alarm;
                }), pending.end());
            }
        );
    }

    bool call_queued()
    {
        if (pending.empty())
            return false;

        pending.front()->call();
        pending.pop_front();
        return true;
    }

private:
    struct AlarmData
    {
        StubNotifyingAlarm const* alarm;
        std::function<void()> call;
    };
    std::list<std::shared_ptr<AlarmData>> pending;
};
}
}
}

#endif // MIR_UNIT_TEST_SHELL_TRANSFORMER_COMMON_H
