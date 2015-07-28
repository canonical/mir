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

#ifndef MIR_TEST_DOUBLES_FAKE_ALARM_FACTORY_H_
#define MIR_TEST_DOUBLES_FAKE_ALARM_FACTORY_H_

#include "mir/time/alarm_factory.h"
#include "mir/time/alarm.h"
#include "mir/time/clock.h"
#include "mir/test/doubles/advanceable_clock.h"

#include <vector>

namespace mt = mir::time;

namespace mir
{
namespace test
{
namespace doubles
{
class FakeAlarm : public mt::Alarm
{
public:
    FakeAlarm(std::function<void()> const& callback,
        std::shared_ptr<mir::time::Clock> const& clock);

    void time_updated();

    bool cancel() override;
    State state() const override;

    bool reschedule_in(std::chrono::milliseconds delay) override;
    bool reschedule_for(mir::time::Timestamp timeout) override;

private:
    std::function<void()> const callback;
    State alarm_state;
    mir::time::Timestamp triggers_at;
    std::shared_ptr<mt::Clock> clock;
};

class FakeAlarmFactory : public mt::AlarmFactory
{
public:
    FakeAlarmFactory();

    std::unique_ptr<mt::Alarm> create_alarm(
        std::function<void()> const& callback) override;
    std::unique_ptr<mt::Alarm> create_alarm(
        std::shared_ptr<mir::LockableCallback> const& callback) override;

    void advance_by(mt::Duration step);

private:
    std::vector<FakeAlarm*> alarms;
    std::shared_ptr<AdvanceableClock> const clock;
};

}
}
}

#endif // MIR_TEST_DOUBLES_FAKE_ALARM_FACTORY_H_
