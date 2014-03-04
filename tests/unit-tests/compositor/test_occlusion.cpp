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

#include "mir/compositor/compositing_criteria.h"
#include "mir/geometry/rectangle.h"
#include "src/server/compositor/occlusion.h"
#include "mir_test_doubles/stub_buffer_stream.h"
#include "mir_test_doubles/stub_compositing_criteria.h"

#include <gtest/gtest.h>
#include <memory>

using namespace testing;
using namespace mir::geometry;
using namespace mir::compositor;
using namespace mir::test::doubles;

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
    OcclusionFilter filter(monitor_rect);

    StubCompositingCriteria win(12, 34, 56, 78);

    EXPECT_FALSE(filter(win));
}

TEST_F(OcclusionFilterTest, smaller_window_occluded)
{
    OcclusionFilter filter(monitor_rect);

    StubCompositingCriteria front(10, 10, 10, 10);
    EXPECT_FALSE(filter(front));

    StubCompositingCriteria back(12, 12, 5, 5);
    EXPECT_TRUE(filter(back));
}

TEST_F(OcclusionFilterTest, translucent_window_occludes_nothing)
{
    OcclusionFilter filter(monitor_rect);

    StubCompositingCriteria front(10, 10, 10, 10, 0.5f);
    EXPECT_FALSE(filter(front));

    StubCompositingCriteria back(12, 12, 5, 5, 1.0f);
    EXPECT_FALSE(filter(back));
}

TEST_F(OcclusionFilterTest, hidden_window_is_self_occluded)
{
    OcclusionFilter filter(monitor_rect);

    StubCompositingCriteria front(10, 10, 10, 10, 1.0f, true, false);
    EXPECT_TRUE(filter(front));
}

TEST_F(OcclusionFilterTest, hidden_window_occludes_nothing)
{
    OcclusionFilter filter(monitor_rect);

    StubCompositingCriteria front(10, 10, 10, 10, 1.0f, true, false);
    EXPECT_TRUE(filter(front));

    StubCompositingCriteria back(12, 12, 5, 5);
    EXPECT_FALSE(filter(back));
}

TEST_F(OcclusionFilterTest, shaped_window_occludes_nothing)
{
    OcclusionFilter filter(monitor_rect);

    StubCompositingCriteria front(10, 10, 10, 10, 1.0f, false, true);
    EXPECT_FALSE(filter(front));

    StubCompositingCriteria back(12, 12, 5, 5);
    EXPECT_FALSE(filter(back));
}

TEST_F(OcclusionFilterTest, identical_window_occluded)
{
    OcclusionFilter filter(monitor_rect);

    StubCompositingCriteria front(10, 10, 10, 10);
    EXPECT_FALSE(filter(front));

    StubCompositingCriteria back(10, 10, 10, 10);
    EXPECT_TRUE(filter(back));
}

TEST_F(OcclusionFilterTest, larger_window_never_occluded)
{
    OcclusionFilter filter(monitor_rect);

    StubCompositingCriteria front(10, 10, 10, 10);
    EXPECT_FALSE(filter(front));

    StubCompositingCriteria back(9, 9, 12, 12);
    EXPECT_FALSE(filter(back));
}

TEST_F(OcclusionFilterTest, cascaded_windows_never_occluded)
{
    OcclusionFilter filter(monitor_rect);

    for (int x = 0; x < 10; x++)
    {
        StubCompositingCriteria win(x, x, 200, 100);
        ASSERT_FALSE(filter(win));
    }
}

TEST_F(OcclusionFilterTest, some_occluded_and_some_not)
{
    OcclusionFilter filter(monitor_rect);

    StubCompositingCriteria front(10, 20, 400, 300);
    EXPECT_FALSE(filter(front));

    EXPECT_TRUE(filter(StubCompositingCriteria(10, 20, 5, 5)));
    EXPECT_TRUE(filter(StubCompositingCriteria(100, 100, 20, 20)));
    EXPECT_TRUE(filter(StubCompositingCriteria(200, 200, 50, 50)));

    EXPECT_FALSE(filter(StubCompositingCriteria(500, 600, 34, 56)));
    EXPECT_FALSE(filter(StubCompositingCriteria(200, 200, 1000, 1000)));
}

TEST(OcclusionMatchTest, remembers_matches)
{
    OcclusionMatch match;
    StubCompositingCriteria win(1, 2, 3, 4);
    StubBufferStream bufs;

    EXPECT_FALSE(match.occluded(win));
    match(win, bufs);
    EXPECT_TRUE(match.occluded(win));
}

