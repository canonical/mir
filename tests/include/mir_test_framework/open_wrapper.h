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
#ifndef MIR_TEST_FRAMEWORK_OPEN_WRAPPER_H_
#define MIR_TEST_FRAMEWORK_OPEN_WRAPPER_H_

#include <functional>
#include <memory>
#include <experimental/optional>
#include <sys/stat.h>

namespace mir_test_framework
{
using OpenHandlerHandle = std::unique_ptr<void, void(*)(void*)>;
using OpenHandler =
    std::function<std::experimental::optional<int>(char const* path, int flags, mode_t mode)>;
/**
 * Add a function to the open() interposer.
 *
 * When code calls ::open() the test framework first checks against all of the registered
 * handlers, returning the value from the first handler to return an occupied optional<int>
 *
 * \note    The new handler is added to the \em start of the handler chain; it will be called
 *          before any existing handler on open().
 *
 * \param handler [in]  Handler to call when open() is called. The handler should return an
 *                          occupied optional<int> only when it wants to claim this invocation.
 * \return  An opaque handle to this instance of the handler. Dropping the handle unregisters the
 *          open() handler.
 */
OpenHandlerHandle add_open_handler(OpenHandler handler);
}

#endif //MIR_TEST_FRAMEWORK_OPEN_WRAPPER_H_
