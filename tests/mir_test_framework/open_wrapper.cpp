/*
 * Copyright © Canonical Ltd.
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


/* Because we're trying to interpose libc functions we get exposed to the
 * implementation details of glibc.
 *
 * Due to the 64-bit time_t transition, our previous workaround of
 * #undef _FILE_OFFSET_BITS (and thus getting the base entrypoints)
 * no longer works; _FILE_OFFSET_BITS must be 64 if _TIME_BITS == 64,
 * and it's hard to verify that none of the subsequent headers *don't*
 * embed a time_t somewhere.
 *
 * These runes are taken from the Ubuntu umockdev patch solving the same
 * problem there.
 */
#include <features.h>
#ifdef __GLIBC__  /* This is messing with glibc internals, and is not
                   * expected to work (or be needed) anywhere else.
                   * (Hello, musl!)
                   */
/* Remove gcc asm aliasing so that our interposed symbols work as expected */
#include <sys/cdefs.h>

#include <stddef.h>
extern "C"
{
extern int __REDIRECT_NTH (__ttyname_r_alias, (int __fd, char *__buf,
                                               size_t __buflen), ttyname_r);
}
#ifdef __REDIRECT
#undef __REDIRECT
#endif
#define __REDIRECT(name, proto, alias) name proto
#ifdef __REDIRECT_NTH
#undef __REDIRECT_NTH
#endif
#define __REDIRECT_NTH(name, proto, alias) name proto __THROW
#endif // __GLIBC__
/*
 * End glibc hackery (although there is a second block below)
 */

#include "mir_test_framework/open_wrapper.h"
#include "mir_test_framework/interposer_helper.h"

#include <atomic>
#include <list>
#include <mutex>
#include <dlfcn.h>
#include <cstdarg>
#include <fcntl.h>
#include <boost/throw_exception.hpp>

/* Second block of glibc hackery
 *
 * Fixup for making a mess with __REDIRECT above
 */
#ifdef __GLIBC__
#ifdef __USE_TIME_BITS64
#define clock_gettime __clock_gettime64
extern "C"
{
extern int clock_gettime(clockid_t clockid, struct timespec *tp);
}
#endif
#endif // __GLIBC__
/*
 * End glibc hackery
 */


namespace mtf = mir_test_framework;

namespace
{
using OpenInterposer = mtf::InterposerHandlers<int, char const*, int, std::optional<mode_t>>;
}

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

