/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
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
 */

#include "mir/test/fake_clock.h"

namespace mt = mir::test;

mt::FakeClock::FakeClock()
    : current_time{std::chrono::nanoseconds::zero()}
{
}

mt::FakeClock::time_point mt::FakeClock::now() const
{
    return time_point{current_time};
}

void mt::FakeClock::register_time_change_callback(std::function<bool(mt::FakeClock::time_point)> cb)
{
    callbacks.push_back(cb);
}

void mt::FakeClock::advance_time_ns(std::chrono::nanoseconds by)
{
    current_time += by;
    auto next = callbacks.begin();
    while (next != callbacks.end())
    {
        if (!(*next)(now()))
        {
            next = callbacks.erase(next);
        }
        else
        {
            next++;
        }
    }
}
