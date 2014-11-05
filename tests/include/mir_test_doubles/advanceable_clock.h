/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
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
 * Authored By: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_ADVANCEABLE_CLOCK_H_
#define MIR_TEST_DOUBLES_ADVANCEABLE_CLOCK_H_

#include "mir/time/steady_clock.h"
#include <mutex>

namespace mir
{
namespace test
{
namespace doubles
{

class AdvanceableClock : public mir::time::Clock
{
public:
    mir::time::Timestamp now() const override
    {
        std::lock_guard<std::mutex> lock{clock_mutex};
        return current_time;
    }

    mir::time::Duration min_wait_until(mir::time::Timestamp) const override
    {
        return mir::time::Duration{0};
    }

    void advance_by(mir::time::Duration step)
    {
        std::lock_guard<std::mutex> lock{clock_mutex};
        current_time += step;
    }

private:
    mutable std::mutex clock_mutex;
    mir::time::Timestamp current_time{
        []
        {
           mir::time::SteadyClock clock;
           return clock.now();
        }()
        };
};

}
}
}

#endif
