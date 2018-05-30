/*
 * Copyright © 2017 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#ifndef MIR_MUTEX_H_
#define MIR_MUTEX_H_

#include <mutex>
#include <condition_variable>
#include <boost/throw_exception.hpp>

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
        : value{value},
          lock{std::move(lock)}
    {
    }
    MutexGuard(MutexGuard&& from) = default;
    ~MutexGuard() noexcept(false)
    {
        if (lock.owns_lock())
        {
            lock.unlock();
        }
    }

    Guarded& operator*()
    {
        return value;
    }
    Guarded* operator->()
    {
        return &value;
    }
private:
    Guarded& value;
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
        return MutexGuard<Guarded>{std::unique_lock<std::mutex>{mutex}, value};
    }

protected:
    std::mutex mutex;
    Guarded value;
};

template<typename Guarded>
class WaitableMutex : public Mutex<Guarded>
{
public:
    using Mutex<Guarded>::Mutex;

    template<typename Predicate, typename Rep, typename Period>
    MutexGuard<Guarded> wait_for(Predicate predicate, std::chrono::duration<Rep, Period> timeout)
    {
        std::unique_lock<std::mutex> lock{this->mutex};
        if (!notifier.wait_for(lock, timeout, [this, &predicate]() { return predicate(this->value); }))
        {
            BOOST_THROW_EXCEPTION((std::runtime_error{"Notification timeout"}));
        }
        return MutexGuard<Guarded>{std::move(lock), this->value};
    }

    void notify_all()
    {
        notifier.notify_all();
    }
private:
    std::condition_variable notifier;
};


}

#endif //MIR_MUTEX_H_
