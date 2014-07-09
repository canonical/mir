/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored By: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir/read_write_mutex.h"

#include <algorithm>

void mir::ReadWriteMutex::read_lock()
{
    auto const my_id = std::this_thread::get_id();

    std::unique_lock<decltype(mutex)> lock{mutex};
    cv.wait(lock, [&]{ return !write_locked; });

    auto const my_count = std::find_if(
        read_locking_threads.begin(),
        read_locking_threads.end(),
        [my_id](ReaderThreadLockCount const& candidate) { return my_id == candidate.id; });

    if (my_count == read_locking_threads.end())
    {
        read_locking_threads.push_back(ReaderThreadLockCount(my_id, 1U));
    }
    else
    {
        ++(my_count->count);
    }
}

void mir::ReadWriteMutex::read_unlock()
{
    auto const my_id = std::this_thread::get_id();

    std::unique_lock<decltype(mutex)> lock{mutex};
    auto const my_count = std::find_if(
        read_locking_threads.begin(),
        read_locking_threads.end(),
        [my_id](ReaderThreadLockCount const& candidate) { return my_id == candidate.id; });

    --(my_count->count);

    cv.notify_all();
}

void mir::ReadWriteMutex::write_lock()
{
    auto const my_id = std::this_thread::get_id();

    std::unique_lock<decltype(mutex)> lock{mutex};
    cv.wait(lock, [&]
        {
            if (write_locked) return false;
            for (auto const& candidate : read_locking_threads)
            {
                if (candidate.id != my_id && candidate.count != 0) return false;
            }
            return true;
        });
    write_locked = true;
}

void mir::ReadWriteMutex::write_unlock()
{
    std::unique_lock<decltype(mutex)> lock{mutex};
    write_locked = false;
    cv.notify_all();
}
