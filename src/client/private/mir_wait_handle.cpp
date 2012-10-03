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

MirWaitHandle::MirWaitHandle() :
    waiting_threads(0),
    guard(),
    wait_condition(),
    waiting_for_result(false) 
{
}

MirWaitHandle::~MirWaitHandle()
{
    // Delay destruction while there are waiting threads
    while (waiting_threads.load()) yield();
}

void MirWaitHandle::result_requested()
{
    unique_lock<mutex> lock(guard);

    while (waiting_for_result)
        wait_condition.wait(lock);

    waiting_for_result = true;
}

void MirWaitHandle::result_received()
{
    lock_guard<mutex> lock(guard);

    waiting_for_result = false;

    wait_condition.notify_all();
}

void MirWaitHandle::wait_for_result()
{
    int tmp = waiting_threads.load();
    while (!waiting_threads.compare_exchange_weak(tmp, tmp + 1)) yield();

    {
        unique_lock<mutex> lock(guard);
        while (waiting_for_result)
            wait_condition.wait(lock);
    }

    tmp = waiting_threads.load();
    while (!waiting_threads.compare_exchange_weak(tmp, tmp - 1)) yield();
}
