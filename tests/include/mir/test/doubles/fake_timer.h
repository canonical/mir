/*
 * Copyright © 2014-2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 *              Alberto Aguirre <alberto.aguirre@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_MOCK_TIMER_H_
#define MIR_TEST_DOUBLES_MOCK_TIMER_H_

#include "mir/time/alarm_factory.h"
#include "mir/test/doubles/advanceable_clock.h"
#include <memory>

namespace mir
{
namespace test
{
namespace doubles
{

class FakeTimer : public mir::time::AlarmFactory
{
public:
    FakeTimer(std::shared_ptr<AdvanceableClock> const& clock);
    std::unique_ptr<time::Alarm> create_alarm(std::function<void()> const& callback) override;
    std::unique_ptr<time::Alarm> create_alarm(std::unique_ptr<LockableCallback> callback) override;
private:
    std::shared_ptr<AdvanceableClock> const clock;
};

}
}
}

#endif // MIR_TEST_DOUBLES_MOCK_TIMER_H_
