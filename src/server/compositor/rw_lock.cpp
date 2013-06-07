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

mc::RWLockWriterBias::RWLockWriterBias()
    : readers(0),
      writers(0),
      waiting_writers(0)
{
}

void mc::RWLockWriterBias::read_lock()
{
    std::unique_lock<std::mutex> lk(mut);
    while ((waiting_writers > 0) || (writers > 0))
    {
        reader_cv.wait(lk);
    }
    readers++;
}

void mc::RWLockWriterBias::read_unlock()
{
    std::unique_lock<std::mutex> lk(mut);
    readers--;
    if (readers == 0)
    {   
        writer_cv.notify_all();
    }
}

void mc::RWLockWriterBias::write_lock()
{    
    std::unique_lock<std::mutex> lk(mut);
    waiting_writers++;
    while ((readers > 0) || (writers > 0))
    {
        writer_cv.wait(lk);
    }
    waiting_writers--;
    writers++;
}

void mc::RWLockWriterBias::write_unlock()
{
    std::unique_lock<std::mutex> lk(mut);
    writers--;

    reader_cv.notify_all();
    if (waiting_writers > 0)
        writer_cv.notify_all();
}
