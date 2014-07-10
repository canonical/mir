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

#ifndef MIR_READ_WRITE_MUTEX_H_
#define MIR_READ_WRITE_MUTEX_H_

#include <condition_variable>
#include <thread>
#include <vector>

namespace mir
{
/** a recursive read-write mutex.
 * Note that a write lock can be acquired if no other threads have a read lock.
 */
class ReadWriteMutex
{
public:
    void read_lock();

    void read_unlock();

    void write_lock();

    void write_unlock();

private:
    std::mutex mutex;
    std::condition_variable cv;
    struct ThreadLockCount
    {
        ThreadLockCount() : id(), count(0) {}
        ThreadLockCount(std::thread::id id, unsigned int count) : id(id), count(count) {}
        std::thread::id id;
        unsigned int count;
    };
    std::vector<ThreadLockCount> read_locking_threads;
    ThreadLockCount write_locking_thread;
};

class ReadLock
{
public:
    explicit ReadLock(ReadWriteMutex& mutex) : mutex(mutex) { mutex.read_lock(); }
    ~ReadLock() { mutex.read_unlock(); }

private:
    ReadWriteMutex& mutex;
};

class WriteLock
{
public:
    explicit WriteLock(ReadWriteMutex& mutex) : mutex(mutex) { mutex.write_lock(); }
    ~WriteLock() { mutex.write_unlock(); }

private:
    ReadWriteMutex& mutex;
};
}

#endif /* MIR_READ_WRITE_MUTEX_H_ */
