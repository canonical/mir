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

#ifndef MIR_COMPOSITOR_RW_LOCK_H_
#define MIR_COMPOSITOR_RW_LOCK_H_

#include <mutex>
#include <condition_variable>

namespace mir
{
namespace compositor
{

class RWLockWriterBias
{
public:
    RWLockWriterBias();
    void rd_lock();
    void rd_unlock();
    void wr_lock();
    void wr_unlock();

private:
    std::mutex mut;
    std::condition_variable reader_cv, writer_cv;
    unsigned int readers;
    unsigned int writers;
    unsigned int waiting_writers;
};

class ReadLock
{
public:
    ReadLock(RWLockWriterBias& rwlock)
        : rwl(rwlock) {}
    void lock()
    {
        rwl.rd_lock();
    }
    void unlock()
    {
        rwl.rd_unlock();
    }
private:
    RWLockWriterBias& rwl;
};

class WriteLock
{
public:
    WriteLock(RWLockWriterBias& rwlock)
        : rwl(rwlock) {}
    void lock()
    {
        rwl.wr_lock();
    }
    void unlock()
    {
        rwl.wr_unlock();
    }
private:
    RWLockWriterBias& rwl;
};


}
}

#endif /* MIR_COMPOSITOR_RW_LOCK_H_ */
