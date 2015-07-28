/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Ricardo Mendoza <ricardo.mendoza@canonical.com>
 */

#ifndef MIR_CLIENT_ATOMIC_CALLBACK_H_
#define MIR_CLIENT_ATOMIC_CALLBACK_H_

#include <functional>
#include <mutex>

namespace mir
{
namespace client
{
template<typename... Args>
class AtomicCallback
{
public:
    AtomicCallback()
        : callback{[](Args...){}}
    {
    }

    ~AtomicCallback() = default;

    void set_callback(std::function<void(Args...)> const& fn)
    {
        std::lock_guard<std::mutex> lk(guard);

        callback = fn;
    }

    void operator()(Args&&... args) const
    {
        std::lock_guard<std::mutex> lk(guard);

        callback(std::forward<Args>(args)...);
    }

private:
    std::mutex mutable guard;
    std::function<void(Args...)> callback;
};
}
}

#endif /* MIR_CLIENT_ATOMIC_CALLBACK_H_ */
