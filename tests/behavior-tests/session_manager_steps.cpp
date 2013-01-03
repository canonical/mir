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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#include "session_management_context.h"

#include <gtest/gtest.h>
#include <cucumber-cpp/defs.hpp>

#include <memory>
#include <map>

#include <cstdio>

namespace mtc = mir::test::cucumber;

GIVEN("^The display-size is (.+)x(.+)$")
{
    REGEX_PARAM(int, width);
    REGEX_PARAM(int, height);
    USING_CONTEXT(mtc::SessionManagementContext, ctx);

    (void)width;
    (void)height;
    
    // TODO: Implement
}

// TODO: Maybe session names should be quoted because of spaces
WHEN("^(.+) is opened in consuming mode$") 
{
    REGEX_PARAM(std::string, session_name);
    
    // TODO: Implement
}

THEN("^(.+) will have size (.+)x(.+)$")
{
    REGEX_PARAM(std::string, session_name);
    REGEX_PARAM(int, expected_width);
    REGEX_PARAM(int, expected_height);
    
    (void)expected_width;
    (void)expected_height;
    // TODO: Implement
    EXPECT_EQ(true, false);
}

