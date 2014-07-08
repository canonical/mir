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
 * Authored by: Daniel van Vugt <daniel.van.vugt@canonical.com>
 */

#include "mir/geometry/rectangle.h"
#include "src/server/compositor/occlusion.h"
#include "mir_test_doubles/fake_renderable.h"

#include <gtest/gtest.h>
#include <memory>

using namespace testing;
using namespace mir::geometry;
using namespace mir::compositor;
namespace mg = mir::graphics;
namespace mtd = mir::test::doubles;

struct OcclusionFilterTest : public Test
{
    OcclusionFilterTest()
    {
        monitor_rect.top_left = {0, 0};
        monitor_rect.size = {1920, 1200};
    }

    Rectangle monitor_rect;
};

TEST_F(OcclusionFilterTest, single_window_not_occluded)
{
    auto window = std::make_shared<mtd::FakeRenderable>(12, 34, 56, 78);
    mg::RenderableList list{window};
 
    filter_occlusions_from(list, monitor_rect);
    ASSERT_EQ(1u, list.size());
    EXPECT_EQ(window, list.front());
}

TEST_F(OcclusionFilterTest, partially_offscreen_still_visible)
{ // Regression test for LP: #1301115
    auto left =   std::make_shared<mtd::FakeRenderable>(-10,   10, 100, 100);
    auto right =  std::make_shared<mtd::FakeRenderable>(1900,  10, 100, 100);
    auto top =    std::make_shared<mtd::FakeRenderable>(500,   -1, 100, 100);
    auto bottom = std::make_shared<mtd::FakeRenderable>(200, 1000, 100, 1000);
    mg::RenderableList list{left, right, top, bottom};
 
    filter_occlusions_from(list, monitor_rect);
    EXPECT_EQ(4u, list.size());
}

TEST_F(OcclusionFilterTest, smaller_window_occluded)
{
    auto top = std::make_shared<mtd::FakeRenderable>(10, 10, 10, 10);
    auto bottom = std::make_shared<mtd::FakeRenderable>(12, 12, 5, 5);
    mg::RenderableList list{bottom, top};

    filter_occlusions_from(list, monitor_rect);

    ASSERT_EQ(1u, list.size());
    EXPECT_EQ(top, list.front());
}

TEST_F(OcclusionFilterTest, translucent_window_occludes_nothing)
{
    auto top = std::make_shared<mtd::FakeRenderable>(Rectangle{{10, 10}, {10, 10}}, 0.5f);
    auto bottom = std::make_shared<mtd::FakeRenderable>(Rectangle{{12, 12}, {5, 5}}, 1.0f);
    mg::RenderableList list{bottom, top};

    filter_occlusions_from(list, monitor_rect);

    ASSERT_EQ(2u, list.size());
    EXPECT_EQ(bottom, list.front());
    EXPECT_EQ(top, list.back());
}

TEST_F(OcclusionFilterTest, hidden_window_is_self_occluded)
{
    auto window = std::make_shared<mtd::FakeRenderable>(Rectangle{{10, 10}, {10, 10}}, 1.0f, true, false);
    mg::RenderableList list{window};

    filter_occlusions_from(list, monitor_rect);

    EXPECT_EQ(0u, list.size());
}

TEST_F(OcclusionFilterTest, hidden_window_occludes_nothing)
{
    auto top = std::make_shared<mtd::FakeRenderable>(Rectangle{{10, 10}, {10, 10}}, 1.0f, true, false);
    auto bottom = std::make_shared<mtd::FakeRenderable>(12, 12, 5, 5);
    mg::RenderableList list{bottom, top};

    filter_occlusions_from(list, monitor_rect);

    ASSERT_EQ(1u, list.size());
    EXPECT_EQ(bottom, list.front());
}

TEST_F(OcclusionFilterTest, shaped_window_occludes_nothing)
{
    auto top = std::make_shared<mtd::FakeRenderable>(Rectangle{{10, 10}, {10, 10}}, 1.0f, false, true);
    auto bottom = std::make_shared<mtd::FakeRenderable>(12, 12, 5, 5);
    mg::RenderableList list{bottom, top};

    filter_occlusions_from(list, monitor_rect);

    ASSERT_EQ(2u, list.size());
    EXPECT_EQ(bottom, list.front());
    EXPECT_EQ(top, list.back());
}

TEST_F(OcclusionFilterTest, identical_window_occluded)
{
    auto top = std::make_shared<mtd::FakeRenderable>(10, 10, 10, 10);
    auto bottom = std::make_shared<mtd::FakeRenderable>(10, 10, 10, 10);
    mg::RenderableList list{bottom, top};

    filter_occlusions_from(list, monitor_rect);

    ASSERT_EQ(1u, list.size());
    EXPECT_EQ(top, list.front());
}

TEST_F(OcclusionFilterTest, larger_window_never_occluded)
{
    auto top = std::make_shared<mtd::FakeRenderable>(10, 10, 10, 10);
    auto bottom = std::make_shared<mtd::FakeRenderable>(9, 9, 12, 12);
    mg::RenderableList list{bottom, top};

    filter_occlusions_from(list, monitor_rect);

    ASSERT_EQ(2u, list.size());
    EXPECT_EQ(bottom, list.front());
    EXPECT_EQ(top, list.back());
}

TEST_F(OcclusionFilterTest, cascaded_windows_never_occluded)
{
    mg::RenderableList list;
    unsigned int const num_windows{10u};
    for (auto x = 0u; x < num_windows; x++)
        list.push_back(std::make_shared<mtd::FakeRenderable>(x, x, 200, 100));

    filter_occlusions_from(list, monitor_rect);
    EXPECT_EQ(num_windows, list.size());
}

TEST_F(OcclusionFilterTest, some_occluded_and_some_not)
{
    auto window0 = std::make_shared<mtd::FakeRenderable>(10, 20, 400, 300);
    auto window1 = std::make_shared<mtd::FakeRenderable>(10, 20, 5, 5);
    auto window2 = std::make_shared<mtd::FakeRenderable>(100, 100, 20, 20);
    auto window3 = std::make_shared<mtd::FakeRenderable>(200, 200, 50, 50);
    auto window4 = std::make_shared<mtd::FakeRenderable>(500, 600, 34, 56);
    auto window5 = std::make_shared<mtd::FakeRenderable>(200, 200, 1000, 1000);
    mg::RenderableList list{
        window5, //not occluded
        window4, //not occluded
        window3, //occluded
        window2, //occluded
        window1, //occluded
        window0  //not occluded
    };

    filter_occlusions_from(list, monitor_rect);

    auto expected_size = 3u;
    ASSERT_EQ(expected_size, list.size());
    auto it = list.begin();
    for(auto count = 0u; count < expected_size; count++)
    {
        switch (count)
        {
            case 0u:
                EXPECT_EQ(window5, *it);
                break;
            case 1u:
                EXPECT_EQ(window4, *it);
                break;
            case 2u:
                EXPECT_EQ(window0, *it);
                break;
            default:
                FAIL();
                break;
        }
        it++;
    }
}
