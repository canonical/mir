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

#include "mir/test/doubles/stub_notifying_alarm.h"
#include "mir/time/clock.h"

namespace mtd = mir::test::doubles;

mtd::StubNotifyingAlarm::StubNotifyingAlarm(
    std::shared_ptr<time::Clock> const& clock,
    std::function<void(StubNotifyingAlarm*, time::Timestamp)> const& on_rescheduled,
    std::function<void(StubNotifyingAlarm const*)> const& on_cancelled) :
    clock{clock},
    on_rescheduled{on_rescheduled},
    on_cancelled{on_cancelled},
    state_{cancelled}
{
}

mir::time::Alarm::State mir::test::doubles::StubNotifyingAlarm::state() const
{
    return state_;
}

mir::test::doubles::StubNotifyingAlarm::~StubNotifyingAlarm()
{
    on_cancelled(this);
}

bool mtd::StubNotifyingAlarm::reschedule_in(std::chrono::milliseconds offset)
{
    return reschedule_for(clock->now() + offset);
}

bool mir::test::doubles::StubNotifyingAlarm::reschedule_for(mir::time::Timestamp timestamp)
{
    on_rescheduled(this, timestamp);
    state_ = pending;
    return true;
}

bool mir::test::doubles::StubNotifyingAlarm::cancel()
{
    if (state_ == pending)
    {
        on_cancelled(this);
        state_ = cancelled;
    }
    return state_ == cancelled;
}

void mir::test::doubles::StubNotifyingAlarm::about_to_be_called()
{
    state_ = triggered;
}
