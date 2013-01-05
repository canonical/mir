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

#include "session_management_context.h"

#include <gtest/gtest.h>
#include <cucumber-cpp/defs.hpp>

#include <memory>
#include <map>

#include <cstdio>

namespace mtc = mir::test::cucumber;
namespace geom = mir::geometry;

GIVEN("^the display-size is (.+)x(.+)$")
{
    REGEX_PARAM(int, width);
    REGEX_PARAM(int, height);
    USING_CONTEXT(mtc::SessionManagementContext, ctx);
    
    ctx->set_view_area(geom::Rectangle{geom::Point(),
                       geom::Size{geom::Width{width}, 
                                  geom::Height{height}}});

}

// TODO: Maybe session names should be quoted to allow for spaces
WHEN("^(.+) is opened in consuming mode$") 
{
    REGEX_PARAM(std::string, window_name);
    USING_CONTEXT(mtc::SessionManagementContext, ctx);
    
    EXPECT_TRUE(ctx->open_window_consuming(window_name));
    
}

WHEN("^(.+) is opened with size (.+)x(.+)$")
{
    REGEX_PARAM(std::string, window_name);
    REGEX_PARAM(int, request_width);
    REGEX_PARAM(int, request_height);
    USING_CONTEXT(mtc::SessionManagementContext, ctx);
    
    EXPECT_TRUE(ctx->open_window_sized(window_name, geom::Size{geom::Width{request_width},
                                                    geom::Height{request_height}}));
    
}

THEN("^(.+) will have size (.+)x(.+)$")
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

