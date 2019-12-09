/*
 * Copyright © 2019 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
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
 * Authored By: William Wold <william.wold@canonical.com>
 */

#ifndef MIR_SHELL_DECORATION_THREADSAFE_ACCESS_H_
#define MIR_SHELL_DECORATION_THREADSAFE_ACCESS_H_

#include "mir/executor.h"
#include "mir/fatal.h"

#include <memory>
#include <mutex>
#include <experimental/optional>

namespace mir
{
namespace shell
{
namespace decoration
{
template<typename T>
class ThreadsafeAccess
    : public std::enable_shared_from_this<ThreadsafeAccess<T>>
{
public:
    ThreadsafeAccess(
        T* target,
        std::shared_ptr<Executor> const& executor)
        : target{target},
          executor{executor}
    {
    }

    ~ThreadsafeAccess()
    {
        if (target)
            fatal_error("ThreadsafeAccess never invalidated");
    }

    void spawn(std::function<void(T*)>&& work)
    {
        executor->spawn([self = this->shared_from_this(), work]()
            {
                std::lock_guard<std::mutex> lock{self->mutex};
                if (self->target)
                    work(self->target.value());
            });
    }

    void invalidate()
    {
        std::lock_guard<std::mutex> lock{mutex};
        target = std::experimental::nullopt;
    }

private:
    std::mutex mutex;
    std::experimental::optional<T* const> target;
    std::shared_ptr<Executor> executor;
};
}
}
}

#endif // MIR_SHELL_DECORATION_THREADSAFE_ACCESS_H_
