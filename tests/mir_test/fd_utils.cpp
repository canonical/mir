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

#include "mir/test/fd_utils.h"

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
                                             << strerror(errno)
                                             << "]";
    }
}

::testing::AssertionResult mir::test::fd_is_readable(mir::Fd const& fd)
{
    return fd_becomes_readable(fd, std::chrono::seconds{0});
}
