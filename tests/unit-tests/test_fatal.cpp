/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Daniel van Vugt <daniel.van.vugt@canonical.com>
 *         Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir/fatal.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <csignal>

using namespace testing;

TEST(FatalErrorDeathTest, abort_formats_message_to_stderr)
{
    mir::FatalErrorStrategy on_error{mir::fatal_error_abort};

    EXPECT_DEATH({mir::fatal_error("%s had %d %s %s", "Mary", 1, "little", "lamb");},
                 "Mary had 1 little lamb");
}

TEST(FatalErrorDeathTest, abort_raises_sigabrt)
{
    mir::FatalErrorStrategy on_error{mir::fatal_error_abort};

    EXPECT_EXIT({mir::fatal_error("Hello world");},
                KilledBySignal(SIGABRT),
                "Hello world");
}

TEST(FatalErrorTest, throw_formats_message_to_what)
{
    EXPECT_THROW(
        mir::fatal_error("%s had %d %s %s", "Mary", 1, "little", "lamb"),
        std::runtime_error);

    try
    {
        mir::fatal_error("%s had %d %s %s", "Mary", 1, "little", "lamb");
    }
    catch (std::exception const& x)
    {
        EXPECT_THAT(x.what(), StrEq("Mary had 1 little lamb"));
    }
}

