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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir_client/mir_wait_handle.h"

MirWaitHandle::MirWaitHandle() :
    guard(),
    wait_condition(),
    result_has_occurred(false)
{
}

MirWaitHandle::~MirWaitHandle()
{
}

void MirWaitHandle::result_received()
{
    unique_lock<mutex> lock(guard);
    result_has_occurred = true;

    wait_condition.notify_all();
}

void MirWaitHandle::wait_for_result()
{
    unique_lock<mutex> lock(guard);
    while ( (!result_has_occurred) )
        wait_condition.wait(lock);
    result_has_occurred = false;
}


