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

#include <list>
#include <mutex>
#include <unistd.h>
#include <dlfcn.h>

namespace mtf = mir_test_framework;

namespace
{
class OpenHandlers
{
public:
    static auto instance() -> OpenHandlers&
    {
        // static local so we don't have to worry about initialization order
        static OpenHandlers open_handlers;
        return open_handlers;
    }

    auto add(mtf::OpenHandler handler) -> mtf::OpenHandlerHandle
    {
        std::lock_guard<std::mutex> lock{mutex};
        auto iterator = handlers.emplace(handlers.begin(), std::move(handler));
        auto remove_callback = +[](void* iterator)
            {
                auto to_remove = static_cast<std::list<mtf::OpenHandler>::iterator*>(iterator);
                instance().remove(to_remove);
            };
        return mtf::OpenHandlerHandle{
            static_cast<void*>(new std::list<mtf::OpenHandler>::iterator{iterator}),
            remove_callback};
    }

    void remove(std::list<mtf::OpenHandler>::iterator* to_remove)
    {
        std::lock_guard<std::mutex> lock{mutex};
        handlers.erase(*to_remove);
        delete to_remove;
    }

    auto run(char const* path, int flags, mode_t mode) -> std::experimental::optional<int>
    {
        std::lock_guard<std::mutex> lock{mutex};
        for (auto const& handler : handlers)
        {
            if (auto val = handler(path, flags, mode))
            {
                return val;
            }
        }
        return {};
    }

private:
    std::mutex mutex;
    std::list<mtf::OpenHandler> handlers;
};
}

mtf::OpenHandlerHandle mtf::add_open_handler(OpenHandler handler)
{
    return OpenHandlers::instance().add(std::move(handler));
}

// We need to explicitly mark this as C because we don't match the
// libc header; we only care about the three-parameter version
extern "C"
{
int open(char const* path, int flags, mode_t mode)
{
    if (auto val = OpenHandlers::instance().run(path, flags, mode))
    {
        return *val;
    }

    int (*real_open)(char const *path, int flags, mode_t mode);
    *(void **)(&real_open) = dlsym(RTLD_NEXT, "open");

    return (*real_open)(path, flags, mode);
}

int open64(char const* path, int flags, mode_t mode)
{
    if (auto val = OpenHandlers::instance().run(path, flags, mode))
    {
        return *val;
    }

    int (*real_open64)(char const *path, int flags, mode_t mode);
    *(void **)(&real_open64) = dlsym(RTLD_NEXT, "open64");

    return (*real_open64)(path, flags, mode);
}

int __open(char const* path, int flags, mode_t mode)
{
    if (auto val = OpenHandlers::instance().run(path, flags, mode))
    {
        return *val;
    }

    int (*real_open)(char const *path, int flags, mode_t mode);
    *(void **)(&real_open) = dlsym(RTLD_NEXT, "__open");

    return (*real_open)(path, flags, mode);
}

int __open64(char const* path, int flags, mode_t mode)
{
    if (auto val = OpenHandlers::instance().run(path, flags, mode))
    {
        return *val;
    }

    int (*real_open64)(char const *path, int flags, mode_t mode);
    *(void **)(&real_open64) = dlsym(RTLD_NEXT, "__open64");

    return (*real_open64)(path, flags, mode);
}

int __open_2(char const* path, int flags)
{
    if (auto val = OpenHandlers::instance().run(path, flags, 0))
    {
        return *val;
    }

    int (*real_open_2)(char const *path, int flags);
    *(void **)(&real_open_2) = dlsym(RTLD_NEXT, "__open_2");

    return (*real_open_2)(path, flags);
}

int __open64_2(char const* path, int flags)
{
    if (auto val = OpenHandlers::instance().run(path, flags, 0))
    {
        return *val;
    }

    int (*real_open64_2)(char const *path, int flags);
    *(void **)(&real_open64_2) = dlsym(RTLD_NEXT, "__open64_2");

    return (*real_open64_2)(path, flags);

}
}

