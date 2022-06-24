/*
 * Copyright © 2017 Canonical Ltd.
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
 */

#ifndef MIR_MUTEX_H_
#define MIR_MUTEX_H_

#include <mutex>

namespace mir
{
/**
 * Smart-pointer-esque accessor for Mutex<> protected data.
 *
 * Ensures exclusive access to the referenced data.
 *
 * \tparam Guarded Type of data guarded by the mutex.
 */
template<typename Guarded>
class MutexGuard
{
public:
    MutexGuard(std::unique_lock<std::mutex>&& lock, Guarded& value)
        : value{&value},
          lock{std::move(lock)}
    {
    }

    ~MutexGuard() noexcept(false)
    {
        if (lock.owns_lock())
        {
            lock.unlock();
        }
    }

    Guarded& operator*()
    {
        return *value;
    }

    Guarded* operator->()
    {
        return value;
    }

    /**
     * Relinquish access to the mutex-guarded data
     *
     * This prevents further access to the contained data and unlocks
     * the associated mutex.
     */
    void drop()
    {
        value = nullptr;
        lock.unlock();
    }

private:
    Guarded* value;
    std::unique_lock<std::mutex> lock;
};

/**
 * A data-locking mutex
 *
 * This is a mutex which owns the data it guards, and can give out a
 * smart-pointer-esque lock to lock and access it.
 *
 * \tparam Guarded  The type of data guarded by the mutex
 */
template<typename Guarded>
class Mutex
{
public:
    Mutex() = default;
    Mutex(Guarded&& initial_value)
        : value{std::move(initial_value)}
    {
    }

    Mutex(Mutex const&) = delete;
    Mutex& operator=(Mutex const&) = delete;

    /**
     * Lock the mutex and return an accessor for the protected data.
     *
     * \return A smart-pointer-esque accessor for the contained data.
     *          While code has access to the MutexGuard it is guaranteed to have exclusive
     *          access to the contained data.
     */
    MutexGuard<Guarded> lock()
    {
        return MutexGuard<Guarded>{std::unique_lock{mutex}, value};
    }

    /**
     * Unlock an acquired mutex
     *
     * This is a convenience method for cases when the mutex must be dropped
     * but cannot easily be made to leave scope.
     */
    static void drop(MutexGuard<Guarded> /*to_drop*/)
    {
    }
private:
    std::mutex mutex;
    Guarded value;
};

}

#endif //MIR_MUTEX_H_
