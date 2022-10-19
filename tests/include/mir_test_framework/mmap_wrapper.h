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
#ifndef MIR_TEST_FRAMEWORK_MMAP_WRAPPER_H_
#define MIR_TEST_FRAMEWORK_MMAP_WRAPPER_H_

#include <functional>
#include <memory>
#include <optional>
#include <sys/mman.h>

namespace mir_test_framework
{
using MmapHandlerHandle = std::unique_ptr<void, void(*)(void*)>;
using MmapHandler =
    std::function<std::optional<void*>(void *addr, size_t length, int prot, int flags, int fd, off_t offset)>;
/**
 * Add a function to the mmap() interposer.
 *
 * When code calls ::mmap() the test framework first checks against all of the registered
 * handlers, returning the value from the first handler to return an occupied optional<void*>
 *
 * \note    The new handler is added to the \em start of the handler chain; it will be called
 *          before any existing handler on mmap().
 *
 * \param handler [in]  Handler to call when mmap() is called. The handler should return an
 *                      occupied optional<void*> only when it wants to claim this invocation.
 * \return  An opaque handle to this instance of the handler. Dropping the handle unregisters the
 *          mmap() handler.
 */
MmapHandlerHandle add_mmap_handler(MmapHandler handler);

using MunmapHandlerHandle = std::unique_ptr<void, void(*)(void*)>;
using MunmapHandler =
    std::function<std::optional<int>(void* addr, size_t length)>;

/**
 * Add a function to the munmap() interposer.
 *
 * When code calls ::munmap() the test framework first checks against all of the registered
 * handlers, returning the value from the first handler to return an occupied optional<int>
 *
 * \note    The new handler is added to the \em start of the handler chain; it will be called
 *          before any existing handler on munmap().
 *
 * \param handler [in]  Handler to call when munmap() is called. The handler should return an
 *                      occupied optional<int> only when it wants to claim this invocation.
 * \return  An opaque handle to this instance of the handler. Dropping the handle unregisters the
 *          munmap() handler.
 */
MunmapHandlerHandle add_munmap_handler(MunmapHandler handler);
}

#endif //MIR_TEST_FRAMEWORK_MMAP_WRAPPER_H_

