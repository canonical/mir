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

#ifndef MIR_TEST_DOUBLES_QUEUED_ALARM_STUB_MAIN_LOOP_H_
#define MIR_TEST_DOUBLES_QUEUED_ALARM_STUB_MAIN_LOOP_H_

#include "mir/test/doubles/stub_main_loop.h"
#include "mir/test/doubles/stub_notifying_alarm.h"

#include <deque>

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
/// This MainLoop is a stub aside from the code that handles the creation of alarms.
/// When an alarm is scheduled, this class will add it to a list of functions that
/// need to be called. When an alarm is removed, this class will remove all functions
/// queued on that alarm from the list so that they are not called.
class QueuedAlarmStubMainLoop : public StubMainLoop
{
public:
    QueuedAlarmStubMainLoop(std::shared_ptr<time::Clock> const& clock);

    std::unique_ptr<mir::time::Alarm> create_alarm(std::function<void()> const& f) override;

    bool call_queued();

private:
    struct AlarmData;
    std::shared_ptr<time::Clock> const clock;
    std::deque<std::shared_ptr<AlarmData>> pending;
};
}
}
}

#endif // MIR_TEST_DOUBLES_QUEUED_ALARM_STUB_MAIN_LOOP_H_

