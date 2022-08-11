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
#include <sys/syscall.h>

namespace mtf = mir_test_framework;

#ifdef SYS_mmap
// 64-bit
#define MMAP_SYSCALL SYS_mmap
#else
// 32-bit
#define MMAP_SYSCALL SYS_mmap2
#endif

namespace
{
using MmapInterposer = mtf::InterposerHandlers<void*, void *, size_t, int, int, int, off_t>;
}

mtf::MmapHandlerHandle mtf::add_mmap_handler(MmapHandler handler)
{
    return MmapInterposer::add(std::move(handler));
}

void* mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset)
{
    if (auto val = MmapInterposer::run(addr, length, prot, flags, fd, offset))
    {
        return *val;
    }

    if (offset)
    {
        // The libc mmap() function we're implementing takes an offset in bytes, but the mmap2 version of the syscall
        // present on 32-bit systems takes an offset in 4kB units. It might be possible to do the right thing, but since
        // this is just for tests and we shouldn't use offset, simply error.
        BOOST_THROW_EXCEPTION((std::runtime_error{"wrapped mmap called with offset"}));
    }

    // We previously loaded the mmap symbol from libc, but this broke on armhf for unknown reasons. Instead, call the
    // syscall it wraps directly.
    return (void*)syscall(MMAP_SYSCALL, addr, length, prot, flags, fd, offset);
}
