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

#include "mir/posix_rw_mutex.h"

#include <system_error>
#include <boost/throw_exception.hpp>

namespace
{
void unlock_rwlock(pthread_rwlock_t& mutex, char const* failure_message)
{
    auto err = pthread_rwlock_unlock(&mutex);
    /*
     * The Mutex concept specifies that m.unlock() does not throw exceptions.
     *
     * However, the Mutex concept *also* specifies that the behaviour if the calling thread
     * does not own the mutex is undefined.
     *
     * The only possible error from pthread_rwlock_unlock is if mutex is invalid, in which case
     * the current thread *cannot* own it, so choose our Undefined Behaviour to be “throws exception”
     */
    if (err == EINVAL)
    {
        BOOST_THROW_EXCEPTION((std::system_error{
            err,
            std::system_category(),
            failure_message}));
    }
}
}

mir::PosixRWMutex::PosixRWMutex()
        : PosixRWMutex(Type::Default)
{
}

mir::PosixRWMutex::PosixRWMutex(Type type)
{
    pthread_rwlockattr_t attr;
    int err;

    err = pthread_rwlockattr_init(&attr);
    if (err != 0)
    {
        BOOST_THROW_EXCEPTION((std::system_error{
            err,
            std::system_category(),
            "Failed to init pthread attrs"}));
    }

// pthread_rwlockattr_setkind_np() does not exist on e.g. musl
#if  _XOPEN_SOURCE >= 500 || _POSIX_C_SOURCE >= 200809L
    int pthread_type;
    switch(type)
    {
        case Type::Default:
            pthread_type = PTHREAD_RWLOCK_DEFAULT_NP;
            break;
        case Type::PreferReader:
            pthread_type = PTHREAD_RWLOCK_PREFER_READER_NP;
            break;
        case Type::PreferWriterNonRecursive:
            pthread_type = PTHREAD_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP;
            break;
#ifndef __clang__
    /*
     * clang detects that the above cases are exhaustive, but gcc doesn't.
     *
     * Leave the default clause out if building with clang so that this becomes a build error
     * if the above cases are *not* exhaustive, and give gcc a default clause to satisfy
     * its uninitialised variable warning.
     */
        default:
            pthread_type = PTHREAD_RWLOCK_DEFAULT_NP;
            break;
#endif
    }

    err = pthread_rwlockattr_setkind_np(&attr, pthread_type);
    if (err != 0)
    {
        BOOST_THROW_EXCEPTION((std::system_error{
            err,
            std::system_category(),
            "Failed to set preferred rw-lock mode"}));
    }
#else
    (void)type;
#endif

    err = pthread_rwlock_init(&mutex, &attr);
    if (err != 0)
    {
        BOOST_THROW_EXCEPTION((std::system_error{
            err,
            std::system_category(),
            "Failed to init rw-lock"}));
    }

    pthread_rwlockattr_destroy(&attr);
}

mir::PosixRWMutex::~PosixRWMutex()
{
    pthread_rwlock_destroy(&mutex);
}

void mir::PosixRWMutex::lock_shared()
{
    auto err = pthread_rwlock_rdlock(&mutex);
    if (err != 0)
    {
        BOOST_THROW_EXCEPTION((std::system_error{
            err,
            std::system_category(),
            "Failed to acquire read lock"}));
    }
}
bool mir::PosixRWMutex::try_lock_shared()
{
    auto err = pthread_rwlock_tryrdlock(&mutex);
    if (err == 0)
    {
        // Lock acquired.
        return true;
    }
    if (err == EBUSY)
    {
        // Lock is contended.
        return false;
    }

    BOOST_THROW_EXCEPTION((std::system_error{
        err,
        std::system_category(),
        "Failed to try-acquire read lock"}));
}

void mir::PosixRWMutex::unlock_shared()
{
    unlock_rwlock(mutex, "Failed to release read lock");
}


/*
 * Mutex concept implementation
 */
void mir::PosixRWMutex::lock()
{
    auto err = pthread_rwlock_wrlock(&mutex);
    if (err != 0)
    {
        BOOST_THROW_EXCEPTION((std::system_error{
            err,
            std::system_category(),
            "Failed to acquire write lock"}));
    }
}
bool mir::PosixRWMutex::try_lock()
{
    auto err = pthread_rwlock_trywrlock(&mutex);
    if (err == 0)
    {
        // Lock acquired.
        return true;
    }
    if (err == EBUSY)
    {
        // Lock is contended.
        return false;
    }

    BOOST_THROW_EXCEPTION((std::system_error{
        err,
        std::system_category(),
        "Failed to try-acquire write lock"}));
}

void mir::PosixRWMutex::unlock()
{
    unlock_rwlock(mutex, "Failed to release write lock");
}
