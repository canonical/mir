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

struct RWLockMembers
{
    RWLockMembers();

    std::mutex mut;
    std::condition_variable reader_cv, writer_cv;
    unsigned int readers;
    unsigned int writers;
    unsigned int waiting_writers;
};

class ReadLock
{
public:
    void lock();
    void unlock();

protected:
    ReadLock(RWLockMembers& lock_data)
     : impl(lock_data) 
    {
    }
private:
    RWLockMembers& impl;
};

class WriteLock
{
public:
    void lock();
    void unlock();
protected:
    WriteLock(RWLockMembers& lock_data)
     : impl(lock_data) 
    {
    }
private:
    RWLockMembers& impl;
};

class RWLockWriterBias : public ReadLock, public WriteLock
{
public:
    RWLockWriterBias();
private:
    RWLockMembers lock_members;
};


}
}

#endif /* MIR_COMPOSITOR_RW_LOCK_H_ */
