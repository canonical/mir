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
 *              Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir_test_doubles/mock_surface_info.h"
#include "mir/input/rectangles_input_region.h"
#include "mir/geometry/rectangle.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mtd = mir::test::doubles;
namespace mi = mir::input;
namespace geom = mir::geometry;

struct InputSurfaceInfo : public testing::Test
{
    void SetUp()
    {
        using namespace testing;
        ON_CALL(primitive_info, size())
            .WillByDefault(Return(primitive_sz));
        ON_CALL(primitive_info, top_left())
            .WillByDefault(Return(primitive_pt));
    }

    geom::Point primitive_pt{geom::X{1}, geom::Y{1}};
    geom::Size primitive_sz{geom::Width{1}, geom::Height{1}};
    mtd::MockSurfaceInfo primitive_info;
};

TEST(InputSurfaceInfo, default_region_is_surface_rectangle)
{
    mi::SurfaceDataController input_info(mt::fake_shared(primitive_info));

    //...
    //.O.
    //...
    for(auto x = 0; x <= 2; x++)
    {
        for(auto y = 0; y <= 2; y++)
        {
            auto contains = input_info.input_region_contains(geom::X{x}, geom::Y{y});
            if (( x == 1) && (y == 1))
            {
                EXPECT_TRUE(contains);
            }
            else
            {
                EXPECT_FALSE(contains);
            }
        }
    }
}

TEST(InputSurfaceInfo, set_input_region)
{
    std::vector<geom::Rectangle> const rectangles = {
        {{geom::X{0}, geom::Y{0}}, {geom::Width{1}, geom::Height{1}}},
        {{geom::X{2}, geom::Y{2}}, {geom::Width{1}, geom::Height{1}}}
    };

    mi::SurfaceDataController input_info(mt::fake_shared(primitive_info));
    input_info.set_additional_regions(rectangles);

    //.....
    //.O...
    //.....
    //...O.
    //.....
    for(auto x = -1; x <= 3; x++)
    {
        for(auto y = -1; y <= 3; y++)
        {
            auto contains = input_info.input_region_contains(geom::X{x}, geom::Y{y});
            if ((x == 0) && (y == 0) ||
                (x == 2) && (y == 2))
            {
                EXPECT_TRUE(contains);
            }
            else
            {
                EXPECT_FALSE(contains);
            }
            }
        }
    }
}
