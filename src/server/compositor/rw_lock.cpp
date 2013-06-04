/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by:
 * Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "rw_lock.h"

namespace mc=mir::compositor;

mc::RWLockMembers::RWLockMembers()
    : readers(0),
      writers(0),
      waiting_writers(0)
{
}

mc::RWLockWriterBias::RWLockWriterBias()
    : ReadLock(lock_members), WriteLock(lock_members)
{
}

void mc::ReadLock::lock()
{
    std::unique_lock<std::mutex> lk(impl.mut);
    while ((impl.waiting_writers > 0) || (impl.writers > 0))
    {
        impl.reader_cv.wait(lk);
    }
    impl.readers++;
}

void mc::ReadLock::unlock()
{
    std::unique_lock<std::mutex> lk(impl.mut);
    impl.readers--;
    if (impl.readers == 0)
    {   
        impl.writer_cv.notify_all();
    }
}

void mc::WriteLock::lock()
{    
    std::unique_lock<std::mutex> lk(impl.mut);
    impl.waiting_writers++;
    while ((impl.readers > 0) || (impl.writers > 0))
    {
        impl.writer_cv.wait(lk);
    }
    impl.waiting_writers--;
    impl.writers++;
}

void mc::WriteLock::unlock()
{
    std::unique_lock<std::mutex> lk(impl.mut);
    impl.writers--;

    impl.reader_cv.notify_all();
    if (impl.waiting_writers > 0)
        impl.writer_cv.notify_all();
}
