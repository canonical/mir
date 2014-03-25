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

#include "src/server/compositor/bypass.h"
#include "mir_test_doubles/mock_display_buffer.h"
#include "mir_test_doubles/fake_renderable.h"

#include <gtest/gtest.h>
#include <memory>

namespace mg=mir::graphics;
namespace geom=mir::geometry;
namespace mc=mir::compositor;
namespace mtd=mir::test::doubles;

struct BypassFilterTest : public testing::Test
{
    geom::Rectangle const primary_monitor{{0, 0},{1920, 1200}};
    geom::Rectangle const secondary_monitor{{1920, 0},{1920, 1200}};
};

TEST_F(BypassFilterTest, nothing_matches_nothing)
{
    mg::RenderableList empty_list{};
    mc::BypassMatcher matcher(primary_monitor);

    EXPECT_EQ(empty_list.end(), std::find_if(empty_list.begin(), empty_list.end(), matcher));
}

#if 0
//osnteoh
TEST_F(BypassFilterTest, small_window_not_bypassed)
{
    mg::RenderableList list{
        std::make_shared<mtd::FakeRenderable>(12, 34, 56, 78)
    };

    EXPECT_EQ(list.end(), mc::find_bypass_buffer_from(list, primary_monitor));
}

TEST_F(BypassFilterTest, single_fullscreen_window_bypassed)
{
    auto window = std::make_shared<mtd::FakeRenderable>(0, 0, 1920, 1200);
    mg::RenderableList list{window};

    EXPECT_EQ(window, *mc::find_bypass_buffer_from(list, primary_monitor));
}

TEST_F(BypassFilterTest, translucent_fullscreen_window_not_bypassed)
{
    mg::RenderableList list{
        std::make_shared<mtd::FakeRenderable>(0, 0, 1920, 1200, 0.5f)
    };

    EXPECT_EQ(list.end(), mc::find_bypass_buffer_from(list, primary_monitor));
}
#endif
#if 0
TEST_F(BypassFilterTest, hidden_fullscreen_window_not_bypassed)
{
    BypassFilter filter(display_buffer[0]);

    FakeRenderable win(0, 0, 1920, 1200, 1.0f, true, false);

    EXPECT_FALSE(filter(win));
    EXPECT_FALSE(filter.fullscreen_on_top());
}

TEST_F(BypassFilterTest, unposted_fullscreen_window_not_bypassed)
{
    BypassFilter filter(display_buffer[0]);

    FakeRenderable win(0, 0, 1920, 1200, 1.0f, true, true, false);

    EXPECT_FALSE(filter(win));
    EXPECT_FALSE(filter.fullscreen_on_top());
}

TEST_F(BypassFilterTest, shaped_fullscreen_window_not_bypassed)
{
    BypassFilter filter(display_buffer[0]);

    FakeRenderable win(0, 0, 1920, 1200, 1.0f, false);

    EXPECT_FALSE(filter(win));
    EXPECT_FALSE(filter.fullscreen_on_top());
}

TEST_F(BypassFilterTest, offset_fullscreen_window_not_bypassed)
{
    BypassFilter filter(display_buffer[0]);

    FakeRenderable win(10, 50, 1920, 1200);

    EXPECT_FALSE(filter(win));
    EXPECT_FALSE(filter.fullscreen_on_top());
}

TEST_F(BypassFilterTest, obscured_fullscreen_window_not_bypassed)
{
    BypassFilter filter(display_buffer[0]);

    FakeRenderable fs(0, 0, 1920, 1200);
    FakeRenderable small(20, 30, 40, 50);

    EXPECT_TRUE(filter(fs));
    EXPECT_FALSE(filter(small));
    EXPECT_FALSE(filter.fullscreen_on_top());
}

TEST_F(BypassFilterTest, translucently_obscured_fullscreen_window_not_bypassed)
{   // Regression test for LP: #1266385
    BypassFilter filter(display_buffer[0]);

    FakeRenderable fs(0, 0, 1920, 1200);
    FakeRenderable small(20, 30, 40, 50, 0.5f);

    EXPECT_TRUE(filter(fs));
    EXPECT_FALSE(filter(small));
    EXPECT_FALSE(filter.fullscreen_on_top());
}

TEST_F(BypassFilterTest, unobscured_fullscreen_window_bypassed)
{
    BypassFilter filter(display_buffer[0]);

    FakeRenderable fs(0, 0, 1920, 1200);
    FakeRenderable small(20, 30, 40, 50);

    EXPECT_FALSE(filter(small));
    EXPECT_TRUE(filter(fs));
    EXPECT_TRUE(filter.fullscreen_on_top());
}

TEST_F(BypassFilterTest, unobscured_fullscreen_alpha_window_not_bypassed)
{
    BypassFilter filter(display_buffer[0]);

    FakeRenderable fs(0, 0, 1920, 1200, 0.9f);
    FakeRenderable small(20, 30, 40, 50);

    EXPECT_FALSE(filter(small));
    EXPECT_FALSE(filter(fs));
    EXPECT_FALSE(filter.fullscreen_on_top());
}

TEST_F(BypassFilterTest, many_fullscreen_windows_only_bypass_top)
{
    BypassFilter filter(display_buffer[0]);

    FakeRenderable a(0, 0, 1920, 1200);
    EXPECT_TRUE(filter(a));
    EXPECT_TRUE(filter.fullscreen_on_top());

    FakeRenderable b(1, 2, 3, 4);
    EXPECT_FALSE(filter(b));
    EXPECT_FALSE(filter.fullscreen_on_top());

    FakeRenderable c(0, 0, 1920, 1200);
    EXPECT_TRUE(filter(c));
    EXPECT_TRUE(filter.fullscreen_on_top());

    FakeRenderable d(5, 6, 7, 8);
    EXPECT_FALSE(filter(d));
    EXPECT_FALSE(filter.fullscreen_on_top());

    FakeRenderable e(0, 0, 1920, 1200);
    EXPECT_TRUE(filter(e));
    EXPECT_TRUE(filter.fullscreen_on_top());

    FakeRenderable f(9, 10, 11, 12);
    EXPECT_FALSE(filter(f));
    EXPECT_FALSE(filter.fullscreen_on_top());

    FakeRenderable g(0, 0, 1920, 1200);
    EXPECT_TRUE(filter(g));
    EXPECT_TRUE(filter.fullscreen_on_top());
}

TEST_F(BypassFilterTest, many_fullscreen_windows_only_bypass_top_rectangular)
{
    BypassFilter filter(display_buffer[0]);

    FakeRenderable a(0, 0, 1920, 1200, 1.0f, false);
    EXPECT_FALSE(filter(a));
    EXPECT_FALSE(filter.fullscreen_on_top());

    FakeRenderable b(1, 2, 3, 4);
    EXPECT_FALSE(filter(b));
    EXPECT_FALSE(filter.fullscreen_on_top());

    FakeRenderable c(0, 0, 1920, 1200);
    EXPECT_TRUE(filter(c));
    EXPECT_TRUE(filter.fullscreen_on_top());

    FakeRenderable d(5, 6, 7, 8);
    EXPECT_FALSE(filter(d));
    EXPECT_FALSE(filter.fullscreen_on_top());

    FakeRenderable e(0, 0, 1920, 1200, 1.0f, true);
    EXPECT_TRUE(filter(e));
    EXPECT_TRUE(filter.fullscreen_on_top());

    FakeRenderable f(9, 10, 11, 12);
    EXPECT_FALSE(filter(f));
    EXPECT_FALSE(filter.fullscreen_on_top());

    FakeRenderable g(0, 0, 1920, 1200, 0.5f, false);
    EXPECT_FALSE(filter(g));
    EXPECT_FALSE(filter.fullscreen_on_top());
}

TEST_F(BypassFilterTest, many_fullscreen_windows_only_bypass_top_visible_posted)
{
    BypassFilter filter(display_buffer[0]);

    FakeRenderable a(0, 0, 1920, 1200, 1.0f, false);
    EXPECT_FALSE(filter(a));
    EXPECT_FALSE(filter.fullscreen_on_top());

    FakeRenderable b(1, 2, 3, 4);
    EXPECT_FALSE(filter(b));
    EXPECT_FALSE(filter.fullscreen_on_top());

    FakeRenderable c(0, 0, 1920, 1200);
    EXPECT_TRUE(filter(c));
    EXPECT_TRUE(filter.fullscreen_on_top());

    FakeRenderable d(5, 6, 7, 8);
    EXPECT_FALSE(filter(d));
    EXPECT_FALSE(filter.fullscreen_on_top());

    FakeRenderable e(0, 0, 1920, 1200, 1.0f, true, false, true);
    EXPECT_FALSE(filter(e));
    EXPECT_FALSE(filter.fullscreen_on_top());

    FakeRenderable f(0, 0, 1920, 1200, 1.0f, true, true, false);
    EXPECT_FALSE(filter(f));
    EXPECT_FALSE(filter.fullscreen_on_top());

    FakeRenderable g(9, 10, 11, 12);
    EXPECT_FALSE(filter(g));
    EXPECT_FALSE(filter.fullscreen_on_top());

    FakeRenderable h(0, 0, 1920, 1200, 1.0f, true, true, true);
    EXPECT_TRUE(filter(h));
    EXPECT_TRUE(filter.fullscreen_on_top());
}

TEST_F(BypassFilterTest, multimonitor_one_bypassed)
{
    BypassFilter left(display_buffer[0]);
    BypassFilter right(display_buffer[1]);

    FakeRenderable fs(1920, 0, 1920, 1200);
    FakeRenderable small(20, 30, 40, 50);

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

    FakeRenderable left_win(0, 0, 1920, 1200);
    FakeRenderable right_win(1920, 0, 1920, 1200);

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

    FakeRenderable big_win(0, 0, 3840, 1200);

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
    FakeRenderable win(1, 2, 3, 4);

    EXPECT_EQ(nullptr, match.topmost_fullscreen());
    match(win);
    EXPECT_EQ(&win, match.topmost_fullscreen());
}

TEST(BypassMatchTest, returns_latest)
{
    BypassMatch match;
    FakeRenderable a(1, 2, 3, 4), b(5, 6, 7, 8), c(9, 10, 11, 12);

    EXPECT_EQ(nullptr, match.topmost_fullscreen());
    match(a);
    EXPECT_EQ(&a, match.topmost_fullscreen());
    match(b);
    EXPECT_EQ(&b, match.topmost_fullscreen());
    match(c);
    EXPECT_EQ(&c, match.topmost_fullscreen());
    EXPECT_EQ(&c, match.topmost_fullscreen());
    EXPECT_EQ(&c, match.topmost_fullscreen());
}
#endif
