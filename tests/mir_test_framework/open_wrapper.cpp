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
 */

/* As suggested by umockdev, _FILE_OFFSET_BITS breaks our open() interposing,
 * as it results in a (platform dependent!) subset of {__open64, __open64_2,
 * __open, __open_2} being defined (not just declared) in the header, causing
 * the build to fail with duplicate definitions.
 */
#undef _FILE_OFFSET_BITS

#include "mir_test_framework/open_wrapper.h"

#include <atomic>
#include <list>
#include <mutex>
#include <dlfcn.h>
#include <cstdarg>
#include <fcntl.h>
#include <boost/throw_exception.hpp>

namespace mtf = mir_test_framework;

namespace
{
using InterposerHandle = std::unique_ptr<void, void(*)(void*)>;

template<typename Returns, typename... Args>
class InterposerHandlers
{
public:
    static auto add(std::function<std::optional<Returns>(Args...)> handler) -> InterposerHandle
    {
        handlers_added.store(true);
        auto& me = instance();
        std::lock_guard lock{me.mutex};
        auto iterator = me.handlers.emplace(me.handlers.begin(), std::move(handler));
        auto remove_callback = [](void* iterator)
            {
                auto to_remove = static_cast<std::list<mtf::OpenHandler>::iterator*>(iterator);
                remove(to_remove);
            };
        return mtf::OpenHandlerHandle{
            static_cast<void*>(new std::list<mtf::OpenHandler>::iterator{iterator}),
            remove_callback};
    }

    static auto run(Args ...args) -> std::optional<Returns>
    {
        if (handlers_added)
        {
            auto& me = instance();
            std::lock_guard lock{me.mutex};
            for (auto const& handler: me.handlers)
            {
                if (auto val = handler(args...))
                {
                    return val;
                }
            }
        }
        return std::nullopt;
    }

private:
    // We want to ensure that run() doesn't call instance() too early (e.g. during coverage setup)
    // as that leads to destruction order issues. (https://github.com/MirServer/mir/issues/2387)
    static std::atomic<bool> handlers_added;

    static auto instance() -> InterposerHandlers&
    {
        // static local so we don't have to worry about initialization order
        static InterposerHandlers open_handlers;
        return open_handlers;
    }

    static void remove(std::list<mtf::OpenHandler>::iterator* to_remove)
    {
        auto& me = instance();
        std::lock_guard lock{me.mutex};
        me.handlers.erase(*to_remove);
        delete to_remove;
    }

    std::mutex mutex;
    std::list<std::function<std::optional<Returns>(Args...)>> handlers;
};

template<typename Returns, typename... Args>
std::atomic<bool> InterposerHandlers<Returns, Args...>::handlers_added{false};
}

using OpenInterposer = InterposerHandlers<int, char const*, int, std::optional<mode_t>>;

mtf::OpenHandlerHandle mtf::add_open_handler(OpenHandler handler)
{
    return OpenInterposer::add(std::move(handler));
}

int open(char const* path, int flags, ...)
{
    std::optional<mode_t> mode_parameter = std::nullopt;

    /* The open() family of functions take a 3rd, mode, parameter iff it might create a file - O_CREAT (which
     * will create the file if it doesn't exist) or O_TMPFILE (which will create a temporary file)
     */
    if (flags & (O_CREAT | O_TMPFILE))
    {
        std::va_list args;
        va_start(args, flags);
        mode_parameter = va_arg(args, mode_t);
        va_end(args);
    }

    if (auto val = OpenInterposer::run(path, flags, mode_parameter))
    {
        return *val;
    }

    int (*real_open)(char const *path, int flags, ...);
    *(void **)(&real_open) = dlsym(RTLD_NEXT, "open");

    if (!real_open)
    {
        using namespace std::literals::string_literals;
        // Oops! What has gone on here?!
        BOOST_THROW_EXCEPTION((std::runtime_error{"Failed to find open() symbol: "s + dlerror()}));
    }

    if (mode_parameter)
    {
        return (*real_open)(path, flags, *mode_parameter);
    }
    return (*real_open)(path, flags);
}

#ifndef open64 // Alpine does weird stuff
int open64(char const* path, int flags, ...)
{
    std::optional<mode_t> mode_parameter = std::nullopt;

    /* The open() family of functions take a 3rd, mode, parameter iff it might create a file - O_CREAT (which
     * will create the file if it doesn't exist) or O_TMPFILE (which will create a temporary file)
     */
    if (flags & (O_CREAT | O_TMPFILE))
    {
        std::va_list args;
        va_start(args, flags);
        mode_parameter = va_arg(args, mode_t);
        va_end(args);
    }

    if (auto val = OpenInterposer::run(path, flags, mode_parameter))
    {
        return *val;
    }

    int (*real_open64)(char const *path, int flags, ...);
    *(void **)(&real_open64) = dlsym(RTLD_NEXT, "open64");

    if (!real_open64)
    {
        using namespace std::literals::string_literals;
        // Oops! What has gone on here?!
        BOOST_THROW_EXCEPTION((std::runtime_error{"Failed to find open64() symbol: "s + dlerror()}));
    }

    if (mode_parameter)
    {
        return (*real_open64)(path, flags, *mode_parameter);
    }
    return (*real_open64)(path, flags);
}
#endif

int __open(char const* path, int flags, ...)
{
    std::optional<mode_t> mode_parameter = std::nullopt;

    /* The open() family of functions take a 3rd, mode, parameter iff it might create a file - O_CREAT (which
     * will create the file if it doesn't exist) or O_TMPFILE (which will create a temporary file)
     */
    if (flags & (O_CREAT | O_TMPFILE))
    {
        std::va_list args;
        va_start(args, flags);
        mode_parameter = va_arg(args, mode_t);
        va_end(args);
    }

    if (auto val = OpenInterposer::run(path, flags, mode_parameter))
    {
        return *val;
    }

    int (*real_open)(char const *path, int flags, ...);
    *(void **)(&real_open) = dlsym(RTLD_NEXT, "__open");

    if (!real_open)
    {
        using namespace std::literals::string_literals;
        // Oops! What has gone on here?!
        BOOST_THROW_EXCEPTION((std::runtime_error{"Failed to find __open() symbol: "s + dlerror()}));
    }
    if (mode_parameter)
    {
        return (*real_open)(path, flags, *mode_parameter);
    }
    return (*real_open)(path, flags);
}

int __open64(char const* path, int flags, ...)
{
    std::optional<mode_t> mode_parameter = std::nullopt;

    /* The open() family of functions take a 3rd, mode, parameter iff it might create a file - O_CREAT (which
     * will create the file if it doesn't exist) or O_TMPFILE (which will create a temporary file)
     */
    if (flags & (O_CREAT | O_TMPFILE))
    {
        std::va_list args;
        va_start(args, flags);
        mode_parameter = va_arg(args, mode_t);
        va_end(args);
    }

    if (auto val = OpenInterposer::run(path, flags, mode_parameter))
    {
        return *val;
    }

    int (*real_open64)(char const *path, int flags, ...);
    *(void **)(&real_open64) = dlsym(RTLD_NEXT, "__open64");

    if (!real_open64)
    {
        using namespace std::literals::string_literals;
        // Oops! What has gone on here?!
        BOOST_THROW_EXCEPTION((std::runtime_error{"Failed to find __open64() symbol: "s + dlerror()}));
    }
    if (mode_parameter)
    {
        return (*real_open64)(path, flags, *mode_parameter);
    }
    return (*real_open64)(path, flags);
}

int __open_2(char const* path, int flags)
{
    if (auto val = OpenInterposer::run(path, flags, 0))
    {
        return *val;
    }

    int (*real_open_2)(char const *path, int flags);
    *(void **)(&real_open_2) = dlsym(RTLD_NEXT, "__open_2");

    if (!real_open_2)
    {
        using namespace std::literals::string_literals;
        // Oops! What has gone on here?!
        BOOST_THROW_EXCEPTION((std::runtime_error{"Failed to find __open_2() symbol: "s + dlerror()}));
    }
    return (*real_open_2)(path, flags);
}

int __open64_2(char const* path, int flags)
{
    if (auto val = OpenInterposer::run(path, flags, 0))
    {
        return *val;
    }

    int (*real_open64_2)(char const *path, int flags);
    *(void **)(&real_open64_2) = dlsym(RTLD_NEXT, "__open64_2");

    if (!real_open64_2)
    {
        using namespace std::literals::string_literals;
        // Oops! What has gone on here?!
        BOOST_THROW_EXCEPTION((std::runtime_error{"Failed to find __open64_2() symbol: "s + dlerror()}));
    }
    return (*real_open64_2)(path, flags);

}

