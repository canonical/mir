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
 */

#include "mir/fatal.h"
#include <gtest/gtest.h>
#include <csignal>

using namespace testing;

TEST(FatalTest, abort_formats_message_to_stderr)
{
    EXPECT_DEATH({mir::abort("%s had %d %s %s", "Mary", 1, "little", "lamb");},
                 "Mary had 1 little lamb");
}

TEST(FatalTest, abort_raises_sigabrt)
{
    EXPECT_EXIT({mir::abort("Hello world");},
                KilledBySignal(SIGABRT),
                "Hello world");
}
