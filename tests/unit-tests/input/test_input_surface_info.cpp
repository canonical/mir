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
#include "mir_test/fake_shared.h"
#include "mir/input/surface_data_storage.h"
#include "mir/geometry/rectangle.h"

#include <algorithm>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mtd = mir::test::doubles;
namespace mt = mir::test;
namespace mi = mir::input;
namespace geom = mir::geometry;

struct InputSurfaceInfo : public testing::Test
{
    void SetUp()
    {
        using namespace testing;
        ON_CALL(primitive_info, size_and_position())
            .WillByDefault(Return(primitive_rect));
    }

    geom::Point primitive_pt{geom::X{1}, geom::Y{1}};
    geom::Size primitive_sz{geom::Width{1}, geom::Height{1}};
    geom::Rectangle primitive_rect{primitive_pt, primitive_sz};
    mtd::MockSurfaceInfo primitive_info;
};

TEST_F(InputSurfaceInfo, rect_comes_from_shared_data)
{
    using namespace testing;

    EXPECT_CALL(primitive_info, size_and_position())
        .Times(AtLeast(1))
        .WillRepeatedly(Return(primitive_rect));
    mi::SurfaceDataStorage input_info(mt::fake_shared(primitive_info));

    EXPECT_EQ(primitive_rect, input_info.size_and_position());
}

// a 1x1 window at (1,1) will get events at (1,1), (1,2) (2,1), (2,2)
TEST_F(InputSurfaceInfo, default_region_is_surface_rectangle)
{
    mi::SurfaceDataStorage input_info(mt::fake_shared(primitive_info));
    std::initializer_list <geom::Point> contained_pt{geom::Point{geom::X{1}, geom::Y{1}},
                                       geom::Point{geom::X{1}, geom::Y{2}},
                                       geom::Point{geom::X{2}, geom::Y{1}},
                                       geom::Point{geom::X{2}, geom::Y{2}}};

    for(auto x = 0; x <= 3; x++)
    {
        for(auto y = 0; y <= 3; y++)
        {
            auto test_pt = geom::Point{geom::X{x}, geom::Y{y}};
            auto contains = input_info.input_region_contains(test_pt);
            if (std::find(contained_pt.begin(), contained_pt.end(), test_pt) != contained_pt.end())
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


TEST_F(InputSurfaceInfo, set_input_region)
{
    std::vector<geom::Rectangle> const rectangles = {
        {{geom::X{0}, geom::Y{0}}, {geom::Width{1}, geom::Height{1}}}, //region0
        {{geom::X{1}, geom::Y{1}}, {geom::Width{1}, geom::Height{1}}}  //region1
    };

    mi::SurfaceDataStorage input_info(mt::fake_shared(primitive_info));
    input_info.set_input_region(rectangles);

    std::initializer_list <geom::Point> contained_pt{//region0 points
                                                     geom::Point{geom::X{0}, geom::Y{0}},
                                                     geom::Point{geom::X{0}, geom::Y{1}},
                                                     geom::Point{geom::X{1}, geom::Y{0}},
                                                     //overlapping point
                                                     geom::Point{geom::X{1}, geom::Y{1}},
                                                     //region1 points
                                                     geom::Point{geom::X{1}, geom::Y{2}},
                                                     geom::Point{geom::X{2}, geom::Y{1}},
                                                     geom::Point{geom::X{2}, geom::Y{2}},
                                                    };

    for(auto x = 0; x <= 3; x++)
    {
        for(auto y = 0; y <= 3; y++)
        {
            auto test_pt = geom::Point{geom::X{x}, geom::Y{y}};
            auto contains = input_info.input_region_contains(test_pt);
            if (std::find(contained_pt.begin(), contained_pt.end(), test_pt) != contained_pt.end())
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
