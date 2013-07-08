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

class ReadLock
{
public:
    void lock() { read_lock(); }
    void unlock() { read_unlock(); }

    virtual ~ReadLock() = default;
protected:
    ReadLock()
    {
    }
private:
    virtual void read_lock() = 0;
    virtual void read_unlock() = 0;

    ReadLock(ReadLock const&) = delete;
    ReadLock& operator=(ReadLock const&) = delete;
};

class WriteLock
{
public:
    void lock() { write_lock(); }
    void unlock() { write_unlock(); }

    virtual ~WriteLock() = default;
protected:
    WriteLock()
    {
    }
private:
    virtual void write_lock() = 0;
    virtual void write_unlock() = 0;

    WriteLock(WriteLock const&) = delete;
    WriteLock& operator=(WriteLock const&) = delete;
};

class RWLockWriterBias : public ReadLock, public WriteLock
{
public:
    RWLockWriterBias();
private:
    void write_lock();
    void write_unlock();
    void read_lock();
    void read_unlock();
    std::mutex mut;
    std::condition_variable reader_cv, writer_cv;
    unsigned int readers;
    unsigned int writers;
    unsigned int waiting_writers;
};

}
}

#endif /* MIR_COMPOSITOR_RW_LOCK_H_ */
