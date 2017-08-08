/*
 * Copyright © 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
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
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#ifndef MIR_POSIX_RW_MUTEX_H_
#define MIR_POSIX_RW_MUTEX_H_

#include <pthread.h>

namespace mir
{
/**
 * Implementation of the Mutex and SharedMutex C++14 concepts via POSIX pthread rwlock
 *
 * The advantages of using this over std::shared_timed_mutex are:
 * a) The type of rwlock can be selected (as per pthread_rwlock_attr_setkind_np)
 * b) As per POSIX, read locks are recursive rather than undefined behaviour
 */
class PosixRWMutex
{
public:
    enum class Type
    {
        Default,
        PreferReader,
        PreferWriterNonRecursive
    };

    PosixRWMutex();
    PosixRWMutex(Type type);
    ~PosixRWMutex();

    PosixRWMutex(PosixRWMutex const&) = delete;
    PosixRWMutex& operator=(PosixRWMutex const&) = delete;

    /*
     * SharedMutex concept implementation
     */
    void lock_shared();
    bool try_lock_shared();
    void unlock_shared();

    /*
     * Mutex concept implementation
     */
    void lock();
    bool try_lock();
    void unlock();

private:
    pthread_rwlock_t mutex;
};
}

#endif //MIR_POSIX_RW_MUTEX_H_
