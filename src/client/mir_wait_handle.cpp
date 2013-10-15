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
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 *              Daniel van Vugt <daniel.van.vugt@canonical.com>
 */

#include "mir_wait_handle.h"

MirWaitHandle::MirWaitHandle() :
    guard(),
    wait_condition(),
    expecting(0),
    received(0)
{
}

MirWaitHandle::~MirWaitHandle()
{
}

void MirWaitHandle::expect_result()
{
    std::unique_lock<std::mutex> lock(guard);

    expecting++;
}

void MirWaitHandle::result_received()
{
    std::unique_lock<std::mutex> lock(guard);

    received++;
    wait_condition.notify_all();
}

void MirWaitHandle::wait_for_all()  // wait for all results you expect
{
    std::unique_lock<std::mutex> lock(guard);

    while ((!expecting && !received) || (received < expecting))
        wait_condition.wait(lock);

    received = 0;
    expecting = 0;
}

void MirWaitHandle::wait_for_pending(std::chrono::milliseconds limit)
{
    using std::chrono::steady_clock;

    std::unique_lock<std::mutex> lock(guard);

    auto time_limit = steady_clock::now() + limit;

    while (received < expecting && steady_clock::now() < time_limit)
        wait_condition.wait_until(lock, time_limit);
}


void MirWaitHandle::wait_for_one()  // wait for any single result
{
    std::unique_lock<std::mutex> lock(guard);

    while (received == 0)
        wait_condition.wait(lock);

    received--;
    if (expecting > 0)
        expecting--;
}

