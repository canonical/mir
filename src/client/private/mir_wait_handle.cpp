/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Thomas Guest <thomas.guest@canonical.com>
 */

#include "mir_client/private/mir_wait_handle.h"

namespace mcl = mir::client;

mcl::MirWaitHandle::MirWaitHandle() :
    waiting_threads(0),
    guard(),
    wait_condition(),
    waiting_for_result(false) 
{
}

mcl::MirWaitHandle::~MirWaitHandle()
{
    // Delay destruction while there are waiting threads
    while (waiting_threads.load()) std::this_thread::yield();
}

void mcl::MirWaitHandle::result_requested()
{
    std::unique_lock<std::mutex> lock(guard);

    while (waiting_for_result)
        wait_condition.wait(lock);

    waiting_for_result = true;
}

void mcl::MirWaitHandle::result_received()
{
    std::lock_guard<std::mutex> lock(guard);

    waiting_for_result = false;

    wait_condition.notify_all();
}

void mcl::MirWaitHandle::wait_for_result()
{
    int tmp = waiting_threads.load();
    while (!waiting_threads.compare_exchange_weak(tmp, tmp + 1)) std::this_thread::yield();

    {
        std::unique_lock<std::mutex> lock(guard);
        while (waiting_for_result)
            wait_condition.wait(lock);
    }

    tmp = waiting_threads.load();
    while (!waiting_threads.compare_exchange_weak(tmp, tmp - 1)) std::this_thread::yield();
}
