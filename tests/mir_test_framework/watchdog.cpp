/*
 * Copyright © 2014 Canonical Ltd.
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

#include "mir_test_framework/watchdog.h"

#include <boost/throw_exception.hpp>

namespace mtf = mir_test_framework;

mtf::WatchDog::WatchDog(std::function<void(void)> killer)
    : done{false},
      killer{killer}
{
}

mtf::WatchDog::~WatchDog() noexcept
{
    if (runner.joinable())
    {
        killer();
        runner.join();
    }
}

void mtf::WatchDog::run(std::function<void(mtf::WatchDog&)> watchee)
{
    if (runner.joinable())
        BOOST_THROW_EXCEPTION(std::logic_error("Attempt to start a second thread of execution on the WatchDog"));
    runner = std::thread{watchee, std::ref(*this)};
}

void mtf::WatchDog::notify_done()
{
    {
        std::unique_lock<decltype(m)> lock(m);
        done = true;
    }
    notifier.notify_all();
}

void mtf::WatchDog::stop()
{
    killer();
    if (runner.joinable())
        runner.join();
}
