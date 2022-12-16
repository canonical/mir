/*
 * Copyright © 2022 Canonical Ltd.
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

#include "mir_test_framework/mmap_wrapper.h"
#include "mir_test_framework/interposer_helper.h"

#include <boost/throw_exception.hpp>
#include <dlfcn.h>

namespace mtf = mir_test_framework;

namespace
{
using MmapInterposer = mtf::InterposerHandlers<void*, void *, size_t, int, int, int, off_t>;
using MunmapInterposer = mtf::InterposerHandlers<int, void *, size_t>;
}

mtf::MmapHandlerHandle mtf::add_mmap_handler(MmapHandler handler)
{
    return MmapInterposer::add(std::move(handler));
}

auto real_mmap_symbol_name() -> char const*
{
#if _FILE_OFFSET_BITS == 64
    // mmap64 is defined everywhere, so even though off_t == off64_t on 64-bit platforms
    // this is still appropriate.
    return "mmap64";
#else
    // This will get us the 32-bit off_t version on 32-bit platforms
    return "mmap";
#endif
}

void* mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset)
{
    if (auto val = MmapInterposer::run(addr, length, prot, flags, fd, offset))
    {
        return *val;
    }

    void* (*real_mmap)(void *addr, size_t length, int prot, int flags, int fd, off_t offset);
    *(void **)(&real_mmap) = dlsym(RTLD_NEXT, real_mmap_symbol_name());

    if (!real_mmap)
    {
        using namespace std::literals::string_literals;
        // Oops! What has gone on here?!
        BOOST_THROW_EXCEPTION((std::runtime_error{"Failed to find mmap() symbol: "s + dlerror()}));
    }
    return (*real_mmap)(addr, length, prot, flags, fd, offset);
}

mtf::MunmapHandlerHandle mtf::add_munmap_handler(MunmapHandler handler)
{
    return MunmapInterposer::add(std::move(handler));
}

int munmap(void *addr, size_t length)
{
    if (auto val = MunmapInterposer::run(addr, length))
    {
        return *val;
    }

    int (*real_munmap)(void *addr, size_t length);
    *(void **)(&real_munmap) = dlsym(RTLD_NEXT, "munmap");

    if (!real_munmap)
    {
        using namespace std::literals::string_literals;
        // Oops! What has gone on here?!
        BOOST_THROW_EXCEPTION((std::runtime_error{"Failed to find munmap() symbol: "s + dlerror()}));
    }
    return (*real_munmap)(addr, length);
}

