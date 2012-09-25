/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir/process/process.h"

#include <gtest/gtest.h>

#if defined(MIR_DEATH_TESTS_ENABLED)
TEST(ProcessDeathTest,
     construction_with_an_invalid_pid_triggers_assertion)
{
    EXPECT_EXIT(
        mir::process::Process p(0),
        ::testing::KilledBySignal(SIGABRT),
        ".*");
}
#endif // defined(MIR_DEATH_TESTS_ENABLED)



