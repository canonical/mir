/*
 * Copyright © 2013 Canonical Ltd.
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

#include "mir_test_cucumber/session_management_context.h"

#include <gtest/gtest.h>
#include <cucumber-cpp/defs.hpp>

namespace mtc = mir::test::cucumber;
namespace geom = mir::geometry;

#define STEP_APPLICATION_NAME_PARAMETER_REGEX "\"(.+)\""
#define STEP_SIZE_PARAMETER_REGEX "(.+)x(.+)$"

GIVEN("^the display-size is " STEP_SIZE_PARAMETER_REGEX "$")
{
    REGEX_PARAM(int, width);
    REGEX_PARAM(int, height);
    USING_CONTEXT(mtc::SessionManagementContext, ctx);
    
    ctx->set_view_area(geom::Rectangle{geom::Point(),
                       geom::Size{geom::Width{width},
                                  geom::Height{height}}});
}

// TODO: Maybe session names should be quoted to allow for spaces
WHEN("^" STEP_APPLICATION_NAME_PARAMETER_REGEX " is opened in consuming mode$")
{
    REGEX_PARAM(std::string, window_name);
    USING_CONTEXT(mtc::SessionManagementContext, ctx);
    
    EXPECT_TRUE(ctx->open_window_consuming(window_name));
}

WHEN("^" STEP_APPLICATION_NAME_PARAMETER_REGEX " is opened with size " STEP_SIZE_PARAMETER_REGEX "$")
{
    REGEX_PARAM(std::string, window_name);
    REGEX_PARAM(int, request_width);
    REGEX_PARAM(int, request_height);
    USING_CONTEXT(mtc::SessionManagementContext, ctx);
    
    EXPECT_TRUE(ctx->open_window_with_size(window_name, geom::Size{geom::Width{request_width},
                                                                   geom::Height{request_height}}));
}

THEN("^" STEP_APPLICATION_NAME_PARAMETER_REGEX " will have size " STEP_SIZE_PARAMETER_REGEX "$")
{
    REGEX_PARAM(std::string, window_name);
    REGEX_PARAM(int, expected_width);
    REGEX_PARAM(int, expected_height);
    USING_CONTEXT(mtc::SessionManagementContext, ctx);
    
    auto expected_size = geom::Size{geom::Width{expected_width},
                                    geom::Height{expected_height}};

    auto actual_size = ctx->get_window_size(window_name);
    EXPECT_EQ(expected_size, actual_size);
}

