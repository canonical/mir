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

namespace mir
{
namespace compositor
{

class RWLockWriterBias
{
public:
    void rd_lock();
    void rd_unlock();
    void wr_lock();
    void wr_unlock();
protected:
    RWLockWriterBias();
};

class ReadLock : public RWLockWriterBias
{
public:
    void lock()
    {
        rd_lock();
    }
    void unlock()
    {
        rd_unlock();
    }
protected:
    ReadLock();
};

class WriteLock : public RWLockWriterBias
{
public:
    void lock()
    {
        wr_lock();
    }
    void unlock()
    {
        wr_unlock();
    }
protected:
    WriteLock();
};

class RWLock : public virtual ReadLock,
               public virtual WriteLock
{
};

}
}

#endif /* MIR_COMPOSITOR_RW_LOCK_H_ */
