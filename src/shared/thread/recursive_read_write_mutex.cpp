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

#include "mir/recursive_read_write_mutex.h"

#include <algorithm>

void mir::RecursiveReadWriteMutex::read_lock()
{
    auto const my_id = std::this_thread::get_id();

    std::unique_lock<decltype(mutex)> lock{mutex};
    cv.wait(lock, [&]{
        return !write_locking_thread.count ||
            write_locking_thread.id == my_id; });

    auto const my_count = std::find_if(
        read_locking_threads.begin(),
        read_locking_threads.end(),
        [my_id](ThreadLockCount const& candidate) { return my_id == candidate.id; });

    if (my_count == read_locking_threads.end())
    {
        read_locking_threads.push_back(ThreadLockCount(my_id, 1U));
    }
    else
    {
        ++(my_count->count);
    }
}

void mir::RecursiveReadWriteMutex::read_unlock()
{
    auto const my_id = std::this_thread::get_id();

    std::lock_guard<decltype(mutex)> lock{mutex};
    auto const my_count = std::find_if(
        read_locking_threads.begin(),
        read_locking_threads.end(),
        [my_id](ThreadLockCount const& candidate) { return my_id == candidate.id; });

    --(my_count->count);

    cv.notify_all();
}

void mir::RecursiveReadWriteMutex::write_lock()
{
    auto const my_id = std::this_thread::get_id();

    std::unique_lock<decltype(mutex)> lock{mutex};
    cv.wait(lock, [&]
        {
            if (write_locking_thread.count &&
                write_locking_thread.id != my_id) return false;
            for (auto const& candidate : read_locking_threads)
            {
                if (candidate.id != my_id && candidate.count != 0) return false;
            }
            return true;
        });

    ++write_locking_thread.count;
    write_locking_thread.id = my_id;
}

void mir::RecursiveReadWriteMutex::write_unlock()
{
    std::lock_guard<decltype(mutex)> lock{mutex};
    --write_locking_thread.count;
    cv.notify_all();
}
