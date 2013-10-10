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
#include "src/server/compositor/bypass.h"
#include "mir_test_doubles/mock_display_buffer.h"
#include "mir_test_doubles/stub_buffer_stream.h"
#include "mir_test_doubles/stub_compositing_criteria.h"

#include <gtest/gtest.h>
#include <memory>

using namespace testing;
using namespace mir::geometry;
using namespace mir::compositor;
using namespace mir::test::doubles;

struct BypassFilterTest : public Test
{
    BypassFilterTest()
    {
        monitor_rect[0].top_left = {0, 0};
        monitor_rect[0].size = {1920, 1200};
        EXPECT_CALL(display_buffer[0], view_area())
            .WillRepeatedly(Return(monitor_rect[0]));

        monitor_rect[1].top_left = {1920, 0};
        monitor_rect[1].size = {1920, 1200};
        EXPECT_CALL(display_buffer[1], view_area())
            .WillRepeatedly(Return(monitor_rect[1]));
    }

    Rectangle monitor_rect[2];
    MockDisplayBuffer display_buffer[2];
};

TEST_F(BypassFilterTest, nothing_matches_nothing)
{
    BypassFilter filter(display_buffer[0]);
    BypassMatch match;

    EXPECT_FALSE(filter.fullscreen_on_top());
    EXPECT_FALSE(match.topmost_fullscreen());
}

TEST_F(BypassFilterTest, small_window_not_bypassed)
{
    BypassFilter filter(display_buffer[0]);

    StubCompositingCriteria win(12, 34, 56, 78);

    EXPECT_FALSE(filter(win));
    EXPECT_FALSE(filter.fullscreen_on_top());
}

TEST_F(BypassFilterTest, single_fullscreen_window_bypassed)
{
    BypassFilter filter(display_buffer[0]);

    StubCompositingCriteria win(0, 0, 1920, 1200);

    EXPECT_TRUE(filter(win));
    EXPECT_TRUE(filter.fullscreen_on_top());
}

TEST_F(BypassFilterTest, translucent_fullscreen_window_not_bypassed)
{
    BypassFilter filter(display_buffer[0]);

    StubCompositingCriteria win(0, 0, 1920, 1200, 0.5f);

    EXPECT_FALSE(filter(win));
    EXPECT_FALSE(filter.fullscreen_on_top());
}

TEST_F(BypassFilterTest, hidden_fullscreen_window_not_bypassed)
{
    BypassFilter filter(display_buffer[0]);

    StubCompositingCriteria win(0, 0, 1920, 1200, 1.0f, true, false);

    EXPECT_FALSE(filter(win));
    EXPECT_FALSE(filter.fullscreen_on_top());
}

TEST_F(BypassFilterTest, unposted_fullscreen_window_not_bypassed)
{
    BypassFilter filter(display_buffer[0]);

    StubCompositingCriteria win(0, 0, 1920, 1200, 1.0f, true, true, false);

    EXPECT_FALSE(filter(win));
    EXPECT_FALSE(filter.fullscreen_on_top());
}

TEST_F(BypassFilterTest, shaped_fullscreen_window_not_bypassed)
{
    BypassFilter filter(display_buffer[0]);

    StubCompositingCriteria win(0, 0, 1920, 1200, 1.0f, false);

    EXPECT_FALSE(filter(win));
    EXPECT_FALSE(filter.fullscreen_on_top());
}

TEST_F(BypassFilterTest, offset_fullscreen_window_not_bypassed)
{
    BypassFilter filter(display_buffer[0]);

    StubCompositingCriteria win(10, 50, 1920, 1200);

    EXPECT_FALSE(filter(win));
    EXPECT_FALSE(filter.fullscreen_on_top());
}

TEST_F(BypassFilterTest, obscured_fullscreen_window_not_bypassed)
{
    BypassFilter filter(display_buffer[0]);

    StubCompositingCriteria fs(0, 0, 1920, 1200);
    StubCompositingCriteria small(20, 30, 40, 50);

    EXPECT_TRUE(filter(fs));
    EXPECT_FALSE(filter(small));
    EXPECT_FALSE(filter.fullscreen_on_top());
}

TEST_F(BypassFilterTest, unobscured_fullscreen_window_bypassed)
{
    BypassFilter filter(display_buffer[0]);

    StubCompositingCriteria fs(0, 0, 1920, 1200);
    StubCompositingCriteria small(20, 30, 40, 50);

    EXPECT_FALSE(filter(small));
    EXPECT_TRUE(filter(fs));
    EXPECT_TRUE(filter.fullscreen_on_top());
}

TEST_F(BypassFilterTest, unobscured_fullscreen_alpha_window_not_bypassed)
{
    BypassFilter filter(display_buffer[0]);

    StubCompositingCriteria fs(0, 0, 1920, 1200, 0.9f);
    StubCompositingCriteria small(20, 30, 40, 50);

    EXPECT_FALSE(filter(small));
    EXPECT_FALSE(filter(fs));
    EXPECT_FALSE(filter.fullscreen_on_top());
}

TEST_F(BypassFilterTest, many_fullscreen_windows_only_bypass_top)
{
    BypassFilter filter(display_buffer[0]);

    StubCompositingCriteria a(0, 0, 1920, 1200);
    EXPECT_TRUE(filter(a));
    EXPECT_TRUE(filter.fullscreen_on_top());

    StubCompositingCriteria b(1, 2, 3, 4);
    EXPECT_FALSE(filter(b));
    EXPECT_FALSE(filter.fullscreen_on_top());

    StubCompositingCriteria c(0, 0, 1920, 1200);
    EXPECT_TRUE(filter(c));
    EXPECT_TRUE(filter.fullscreen_on_top());

    StubCompositingCriteria d(5, 6, 7, 8);
    EXPECT_FALSE(filter(d));
    EXPECT_FALSE(filter.fullscreen_on_top());

    StubCompositingCriteria e(0, 0, 1920, 1200);
    EXPECT_TRUE(filter(e));
    EXPECT_TRUE(filter.fullscreen_on_top());

    StubCompositingCriteria f(9, 10, 11, 12);
    EXPECT_FALSE(filter(f));
    EXPECT_FALSE(filter.fullscreen_on_top());

    StubCompositingCriteria g(0, 0, 1920, 1200);
    EXPECT_TRUE(filter(g));
    EXPECT_TRUE(filter.fullscreen_on_top());
}

TEST_F(BypassFilterTest, many_fullscreen_windows_only_bypass_top_rectangular)
{
    BypassFilter filter(display_buffer[0]);

    StubCompositingCriteria a(0, 0, 1920, 1200, 1.0f, false);
    EXPECT_FALSE(filter(a));
    EXPECT_FALSE(filter.fullscreen_on_top());

    StubCompositingCriteria b(1, 2, 3, 4);
    EXPECT_FALSE(filter(b));
    EXPECT_FALSE(filter.fullscreen_on_top());

    StubCompositingCriteria c(0, 0, 1920, 1200);
    EXPECT_TRUE(filter(c));
    EXPECT_TRUE(filter.fullscreen_on_top());

    StubCompositingCriteria d(5, 6, 7, 8);
    EXPECT_FALSE(filter(d));
    EXPECT_FALSE(filter.fullscreen_on_top());

    StubCompositingCriteria e(0, 0, 1920, 1200, 1.0f, true);
    EXPECT_TRUE(filter(e));
    EXPECT_TRUE(filter.fullscreen_on_top());

    StubCompositingCriteria f(9, 10, 11, 12);
    EXPECT_FALSE(filter(f));
    EXPECT_FALSE(filter.fullscreen_on_top());

    StubCompositingCriteria g(0, 0, 1920, 1200, 0.5f, false);
    EXPECT_FALSE(filter(g));
    EXPECT_FALSE(filter.fullscreen_on_top());
}

TEST_F(BypassFilterTest, many_fullscreen_windows_only_bypass_top_visible_posted)
{
    BypassFilter filter(display_buffer[0]);

    StubCompositingCriteria a(0, 0, 1920, 1200, 1.0f, false);
    EXPECT_FALSE(filter(a));
    EXPECT_FALSE(filter.fullscreen_on_top());

    StubCompositingCriteria b(1, 2, 3, 4);
    EXPECT_FALSE(filter(b));
    EXPECT_FALSE(filter.fullscreen_on_top());

    StubCompositingCriteria c(0, 0, 1920, 1200);
    EXPECT_TRUE(filter(c));
    EXPECT_TRUE(filter.fullscreen_on_top());

    StubCompositingCriteria d(5, 6, 7, 8);
    EXPECT_FALSE(filter(d));
    EXPECT_FALSE(filter.fullscreen_on_top());

    StubCompositingCriteria e(0, 0, 1920, 1200, 1.0f, true, false, true);
    EXPECT_FALSE(filter(e));
    EXPECT_FALSE(filter.fullscreen_on_top());

    StubCompositingCriteria f(0, 0, 1920, 1200, 1.0f, true, true, false);
    EXPECT_FALSE(filter(f));
    EXPECT_FALSE(filter.fullscreen_on_top());

    StubCompositingCriteria g(9, 10, 11, 12);
    EXPECT_FALSE(filter(g));
    EXPECT_FALSE(filter.fullscreen_on_top());

    StubCompositingCriteria h(0, 0, 1920, 1200, 1.0f, true, true, true);
    EXPECT_TRUE(filter(h));
    EXPECT_TRUE(filter.fullscreen_on_top());
}

TEST_F(BypassFilterTest, multimonitor_one_bypassed)
{
    BypassFilter left(display_buffer[0]);
    BypassFilter right(display_buffer[1]);

    StubCompositingCriteria fs(1920, 0, 1920, 1200);
    StubCompositingCriteria small(20, 30, 40, 50);

    EXPECT_FALSE(left(small));
    EXPECT_FALSE(left.fullscreen_on_top());
    EXPECT_FALSE(left(fs));
    EXPECT_FALSE(left.fullscreen_on_top());

    EXPECT_FALSE(right(small));
    EXPECT_FALSE(right.fullscreen_on_top());
    EXPECT_TRUE(right(fs));
    EXPECT_TRUE(right.fullscreen_on_top());
    EXPECT_FALSE(right(small));
    EXPECT_TRUE(right.fullscreen_on_top());
}

TEST_F(BypassFilterTest, dual_bypass)
{
    BypassFilter left_filter(display_buffer[0]);
    BypassFilter right_filter(display_buffer[1]);

    StubCompositingCriteria left_win(0, 0, 1920, 1200);
    StubCompositingCriteria right_win(1920, 0, 1920, 1200);

    EXPECT_TRUE(left_filter(left_win));
    EXPECT_TRUE(left_filter.fullscreen_on_top());
    EXPECT_FALSE(left_filter(right_win));
    EXPECT_TRUE(left_filter.fullscreen_on_top());

    EXPECT_FALSE(right_filter(left_win));
    EXPECT_FALSE(right_filter.fullscreen_on_top());
    EXPECT_TRUE(right_filter(right_win));
    EXPECT_TRUE(right_filter.fullscreen_on_top());
}

TEST_F(BypassFilterTest, multimonitor_oversized_no_bypass)
{
    BypassFilter left_filter(display_buffer[0]);
    BypassFilter right_filter(display_buffer[1]);

    StubCompositingCriteria big_win(0, 0, 3840, 1200);

    EXPECT_FALSE(left_filter(big_win));
    EXPECT_FALSE(left_filter.fullscreen_on_top());

    EXPECT_FALSE(right_filter(big_win));
    EXPECT_FALSE(right_filter.fullscreen_on_top());
}

TEST(BypassMatchTest, defaults_to_null)
{
    BypassMatch match;

    EXPECT_EQ(nullptr, match.topmost_fullscreen());
}

TEST(BypassMatchTest, returns_one)
{
    BypassMatch match;
    StubCompositingCriteria win(1, 2, 3, 4);
    StubBufferStream bufs;

    EXPECT_EQ(nullptr, match.topmost_fullscreen());
    match(win, bufs);
    EXPECT_EQ(&bufs, match.topmost_fullscreen());
}

TEST(BypassMatchTest, returns_latest)
{
    BypassMatch match;
    StubCompositingCriteria a(1, 2, 3, 4), b(5, 6, 7, 8), c(9, 10, 11, 12);
    StubBufferStream abuf, bbuf, cbuf;

    EXPECT_EQ(nullptr, match.topmost_fullscreen());
    match(a, abuf);
    EXPECT_EQ(&abuf, match.topmost_fullscreen());
    match(b, bbuf);
    EXPECT_EQ(&bbuf, match.topmost_fullscreen());
    match(c, cbuf);
    EXPECT_EQ(&cbuf, match.topmost_fullscreen());
    EXPECT_EQ(&cbuf, match.topmost_fullscreen());
    EXPECT_EQ(&cbuf, match.topmost_fullscreen());
}

