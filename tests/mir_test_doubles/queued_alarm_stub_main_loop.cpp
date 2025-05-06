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

#include "mir/test/doubles/queued_alarm_stub_main_loop.h"
#include "mir/time/clock.h"

namespace mtd = mir::test::doubles;

struct mtd::QueuedAlarmStubMainLoop::AlarmData
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

mtd::QueuedAlarmStubMainLoop::QueuedAlarmStubMainLoop(std::shared_ptr<time::Clock> const& clock) :
    clock{clock}
{
}

std::unique_ptr<mir::time::Alarm> mir::test::doubles::QueuedAlarmStubMainLoop::create_alarm(
    std::function<void()> const& f)
{
    return std::make_unique<StubNotifyingAlarm>(
        clock,
        [this, f = f](StubNotifyingAlarm* alarm, time::Timestamp execution_time)
        {
            pending.push_back(
                std::make_shared<AlarmData>(
                    execution_time,
                    alarm,
                    [f = std::move(f), alarm]
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
                    [alarm](std::shared_ptr<AlarmData> const& data) { return data->alarm == alarm; }),
                pending.end());
        });
}

bool mtd::QueuedAlarmStubMainLoop::call_queued()
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
