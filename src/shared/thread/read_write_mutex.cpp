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
 * Authored By: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir/read_write_mutex.h"


void mir::ReadWriteMutex::read_lock()
{
    std::unique_lock<decltype(mutex)> lock{mutex};
    cv.wait(lock, [&]{ return !write_locked; });
    read_locking_threads.insert(std::this_thread::get_id());
}

void mir::ReadWriteMutex::read_unlock()
{
    std::unique_lock<decltype(mutex)> lock{mutex};
    read_locking_threads.erase(
        read_locking_threads.find(std::this_thread::get_id()));

    cv.notify_all();
}

void mir::ReadWriteMutex::write_lock()
{
    std::unique_lock<decltype(mutex)> lock{mutex};
    cv.wait(lock, [&]
        {
            return !write_locked &&
                    read_locking_threads.count(std::this_thread::get_id())
                    == read_locking_threads.size();
        });
    write_locked = true;
}

void mir::ReadWriteMutex::write_unlock()
{
    std::unique_lock<decltype(mutex)> lock{mutex};
    write_locked = false;
    cv.notify_all();
}
