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

#include "mir_test_framework/open_wrapper.h"

#include <vector>
#include <mutex>
#include <unistd.h>
#include <dlfcn.h>

namespace mtf = mir_test_framework;

namespace
{
std::mutex handlers_mutex;
std::vector<mtf::OpenHandler> handlers;

void remove_handler(void* iterator)
{
    auto to_remove = static_cast<decltype(handlers)::iterator*>(iterator);
    std::lock_guard<std::mutex> lock{handlers_mutex};
    handlers.erase(*to_remove);
    delete to_remove;
}

mtf::OpenHandlerHandle add_handler(mtf::OpenHandler handler)
{
    std::lock_guard<std::mutex> lock{handlers_mutex};
    return mtf::OpenHandlerHandle{
        static_cast<void*>(new decltype(handlers)::iterator{
            handlers.emplace(handlers.begin(), std::move(handler))
        }),
        &remove_handler};
}

std::experimental::optional<int> run_handlers(char const* path, int flags, mode_t mode)
{
    std::lock_guard<std::mutex> lock{handlers_mutex};

    for (auto const& handler : handlers)
    {
        if (auto val = handler(path, flags, mode))
        {
            return val;
        }
    }
    return {};
}
}

mtf::OpenHandlerHandle mtf::add_open_handler(OpenHandler handler)
{
    return add_handler(std::move(handler));
}

// We need to explicitly mark this as C because we don't match the
// libc header; we only care about the three-parameter version
extern "C"
{
int open(char const* path, int flags, mode_t mode)
{
    if (auto val = run_handlers(path, flags, mode))
    {
        return *val;
    }

    int (*real_open)(char const *path, int flags, mode_t mode);
    *(void **)(&real_open) = dlsym(RTLD_NEXT, "open");

    return (*real_open)(path, flags, mode);
}

int open64(char const* path, int flags, mode_t mode)
{
    if (auto val = run_handlers(path, flags, mode))
    {
        return *val;
    }

    int (*real_open64)(char const *path, int flags, mode_t mode);
    *(void **)(&real_open64) = dlsym(RTLD_NEXT, "open64");

    return (*real_open64)(path, flags, mode);
}

int __open(char const* path, int flags, mode_t mode)
{
    if (auto val = run_handlers(path, flags, mode))
    {
        return *val;
    }

    int (*real_open)(char const *path, int flags, mode_t mode);
    *(void **)(&real_open) = dlsym(RTLD_NEXT, "__open");

    return (*real_open)(path, flags, mode);
}

int __open64(char const* path, int flags, mode_t mode)
{
    if (auto val = run_handlers(path, flags, mode))
    {
        return *val;
    }

    int (*real_open64)(char const *path, int flags, mode_t mode);
    *(void **)(&real_open64) = dlsym(RTLD_NEXT, "__open64");

    return (*real_open64)(path, flags, mode);
}

int __open_2(char const* path, int flags)
{
    if (auto val = run_handlers(path, flags, 0))
    {
        return *val;
    }

    int (*real_open_2)(char const *path, int flags);
    *(void **)(&real_open_2) = dlsym(RTLD_NEXT, "__open_2");

    return (*real_open_2)(path, flags);
}

int __open64_2(char const* path, int flags)
{
    if (auto val = run_handlers(path, flags, 0))
    {
        return *val;
    }

    int (*real_open64_2)(char const *path, int flags);
    *(void **)(&real_open64_2) = dlsym(RTLD_NEXT, "__open64_2");

    return (*real_open64_2)(path, flags);

}
}

