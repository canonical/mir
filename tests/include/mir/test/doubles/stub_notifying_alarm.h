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

#ifndef MIR_TEST_DOUBLES_STUB_NOTIFYING_ALARM_H_
#define MIR_TEST_DOUBLES_STUB_NOTIFYING_ALARM_H_

#include "mir/test/doubles/stub_alarm.h"

#include <functional>

namespace mir
{
namespace time
{
class Clock;
}
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
        std::function<void(StubNotifyingAlarm const*)> const& on_cancelled);

    State state() const override;

    ~StubNotifyingAlarm() override;

    bool reschedule_in(std::chrono::milliseconds offset) override;

    bool reschedule_for(mir::time::Timestamp timestamp) override;

    bool cancel() override;

    void about_to_be_called();

private:
    std::shared_ptr<time::Clock> const clock;
    std::function<void(StubNotifyingAlarm*, time::Timestamp)> on_rescheduled;
    std::function<void(StubNotifyingAlarm const*)> on_cancelled;
    State state_;
};
}
}
}

#endif // MIR_TEST_DOUBLES_STUB_NOTIFYING_ALARM_H_
