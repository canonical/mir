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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "mir/test/spin_wait.h"

#include <thread>

bool mir::test::spin_wait_for_condition_or_timeout(
    std::function<bool()> const& condition,
    std::chrono::milliseconds timeout,
    std::chrono::milliseconds spin_period)
{
    auto const end = std::chrono::steady_clock::now() + timeout;
    
    while (std::chrono::steady_clock::now() < end && !condition())
    {
        std::this_thread::sleep_for(spin_period);
    }   

    return condition();
}
