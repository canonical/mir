/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
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

#ifndef MIR_TEST_FD_UTILS_H_
#define MIR_TEST_FD_UTILS_H_

#include "mir/fd.h"
#include <chrono>

#include <poll.h>

#include <gtest/gtest.h>

namespace mir
{
namespace test
{
::testing::AssertionResult std_call_succeeded(int retval);

::testing::AssertionResult fd_is_readable(mir::Fd const& fd);

template<typename Period, typename Rep>
::testing::AssertionResult fd_becomes_readable(mir::Fd const& fd,
                                               std::chrono::duration<Period, Rep> timeout)
{
    int timeout_ms = std::chrono::duration_cast<std::chrono::milliseconds>(timeout).count();

    pollfd readable;
    readable.events = POLLIN;
    readable.fd = fd;

    auto result = std_call_succeeded(poll(&readable, 1, timeout_ms));
    if (result)
    {
        if (readable.revents & POLLERR)
        {
            return ::testing::AssertionFailure() << "error condition on fd";
        }
        if (readable.revents & POLLNVAL)
        {
            return ::testing::AssertionFailure() << "fd is invalid";
        }
        if (!(readable.revents & POLLIN))
        {
            return ::testing::AssertionFailure() << "fd is not readable";
        }
        return ::testing::AssertionSuccess();
    }
    return result;
}
}
}

#endif // MIR_TEST_FD_UTILS_H_
