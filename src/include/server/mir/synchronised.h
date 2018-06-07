/*
 * Copyright © 2018 Canonical Ltd.
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

#ifndef MIR_SYNCHRONISED_H_
#define MIR_SYNCHRONISED_H_

#include <mutex>
#include <condition_variable>
#include <boost/throw_exception.hpp>

namespace mir
{
/**
 * Provides thread-safe access to owned data
 *
 * This is a mutex which owns the data it guards, and can give out a
 * smart-pointer-esque lock to lock and access it.
 *
 * \tparam Guarded  The type of data guarded by the mutex
 */
template<typename Guarded>
class Synchronised
{
public:
    Synchronised() = default;
    Synchronised(Guarded&& initial_value)
        : value{std::move(initial_value)}
    {
    }

    /**
     * RAII wrapper for access to the data owned by a Synchronised type.
     *
     * As long as this object is live the data may only be accessed through it;
     * other attempts to access the data are excluded.
     */
    class Locked
    {
    public:
        friend class Synchronised<Guarded>;

        Locked(Locked&& from) = default;
        ~Locked() noexcept(false)
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
        Locked(std::unique_lock<std::mutex>&& lock, Guarded& value)
            : value{value},
              lock{std::move(lock)}
        {
        }

        Guarded& value;
        std::unique_lock<std::mutex> lock;
    };

    /**
     * Lock the mutex and return an accessor for the protected data.
     *
     * \return A smart-pointer-esque accessor for the contained data.
     *          While code has access to the Locked it is guaranteed to have exclusive
     *          access to the contained data.
     */
    Locked lock()
    {
        return Locked{std::unique_lock<std::mutex>{mutex}, value};
    }

protected:
    std::mutex mutex;
    Guarded value;

private:
    Synchronised(Synchronised const&) = delete;
    Synchronised& operator=(Synchronised const&) = delete;
};

}

#endif //MIR_SYNCHRONISED_H_
