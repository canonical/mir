/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored by: Daniel van Vugt <daniel.van.vugt@canonical.com>
 */

#include "cutils/log.h"  // Get the final value of LOG_NDEBUG
#include <gtest/gtest.h>

TEST(Macros, android_logging_is_disabled)
{
    // Ensure Android logging is disabled. It requires significant
    // CPU time to constantly format messages...
    // This is a regression test for performance bug LP: #1343074.
    EXPECT_EQ(1, LOG_NDEBUG);
}
