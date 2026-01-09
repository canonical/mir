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

#include <mir/errno_utils.h>
#include <mir/test/fd_utils.h>

::testing::AssertionResult mir::test::std_call_succeeded(int retval)
{
    if (retval >= 0)
    {
        return ::testing::AssertionSuccess();
    }
    else
    {
        return ::testing::AssertionFailure() << "errno: "
                                             << errno
                                             << " ["
                                             << mir::errno_to_cstr(errno)
                                             << "]";
    }
}

::testing::AssertionResult mir::test::fd_is_readable(mir::Fd const& fd)
{
    return fd_becomes_readable(fd, std::chrono::seconds{0});
}
