/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <mir/fatal.h>
#include <mir/test/death.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <csignal>

using namespace testing;

TEST(FatalErrorDeathTest, fatal_error_formats_message_to_stderr)
{
    MIR_EXPECT_DEATH(mir::fatal_error("%s had %d %s %s",
                                      "Mary", 1, "little", "lamb"),
                     "Mary had 1 little lamb");
}

TEST(FatalErrorDeathTest, fatal_error_raises_sigabrt)
{
    MIR_EXPECT_EXIT(mir::fatal_error("Hello world"),
                    KilledBySignal(SIGABRT),
                    "Hello world");
}
