/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 *              Daniel van Vugt <daniel.van.vugt@canonical.com>
 */

#include "mir_wait_handle.h"

mir_toolkit::MirWaitHandle::MirWaitHandle() :
    guard(),
    wait_condition(),
    expecting(0),
    received(0)
{
}

mir_toolkit::MirWaitHandle::~MirWaitHandle()
{
}

void mir_toolkit::MirWaitHandle::expect_result()
{
    std::unique_lock<std::mutex> lock(guard);

    expecting++;
}

void mir_toolkit::MirWaitHandle::result_received()
{
    std::unique_lock<std::mutex> lock(guard);

    received++;
    wait_condition.notify_all();
}

void mir_toolkit::MirWaitHandle::wait_for_result()
{
    std::unique_lock<std::mutex> lock(guard);

    while ((!expecting && !received) || (received < expecting))
        wait_condition.wait(lock);

    received = 0;
    expecting = 0;
}

