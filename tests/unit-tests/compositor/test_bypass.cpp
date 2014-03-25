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

TEST_F(BypassFilterTest, small_window_not_bypassed)
{
    mc::BypassMatcher matcher(primary_monitor);
    mg::RenderableList list{
        std::make_shared<mtd::FakeRenderable>(12, 34, 56, 78)
    };

    EXPECT_EQ(list.end(), std::find_if(list.begin(), list.end(), matcher));
}

TEST_F(BypassFilterTest, single_fullscreen_window_bypassed)
{
    auto window = std::make_shared<mtd::FakeRenderable>(0, 0, 1920, 1200);
    mc::BypassMatcher matcher(primary_monitor);
    mg::RenderableList list{window};

    auto it = std::find_if(list.begin(), list.end(), matcher);
    EXPECT_NE(list.end(), it);
    EXPECT_EQ(window, *it);
}

TEST_F(BypassFilterTest, translucent_fullscreen_window_not_bypassed)
{
    mc::BypassMatcher matcher(primary_monitor);
    mg::RenderableList list{
        std::make_shared<mtd::FakeRenderable>(0, 0, 1920, 1200, 0.5f)
    };

    EXPECT_EQ(list.end(), std::find_if(list.begin(), list.end(), matcher));
}

TEST_F(BypassFilterTest, hidden_fullscreen_window_not_bypassed)
{
    mc::BypassMatcher matcher(primary_monitor);
    mg::RenderableList list{
        std::make_shared<mtd::FakeRenderable>(0, 0, 1920, 1200, 1.0f, true, false)
    };

    EXPECT_EQ(list.end(), std::find_if(list.begin(), list.end(), matcher));
}

TEST_F(BypassFilterTest, unposted_fullscreen_window_not_bypassed)
{
    mc::BypassMatcher matcher(primary_monitor);
    mg::RenderableList list{
        std::make_shared<mtd::FakeRenderable>(0, 0, 1920, 1200, 1.0f, true, true, false)
    };

    EXPECT_EQ(list.end(), std::find_if(list.begin(), list.end(), matcher));
}

TEST_F(BypassFilterTest, shaped_fullscreen_window_not_bypassed)
{
    mc::BypassMatcher matcher(primary_monitor);

    mg::RenderableList list{
        std::make_shared<mtd::FakeRenderable>(0, 0, 1920, 1200, 1.0f, false)
    };

    EXPECT_EQ(list.end(), std::find_if(list.begin(), list.end(), matcher));
}

TEST_F(BypassFilterTest, offset_fullscreen_window_not_bypassed)
{
    mc::BypassMatcher matcher(primary_monitor);

    mg::RenderableList list{
        std::make_shared<mtd::FakeRenderable>(10, 50, 1920, 1200)
    };

    EXPECT_EQ(list.end(), std::find_if(list.begin(), list.end(), matcher));
}

TEST_F(BypassFilterTest, obscured_fullscreen_window_not_bypassed)
{
    mc::BypassMatcher matcher(primary_monitor);

    mg::RenderableList list{
        std::make_shared<mtd::FakeRenderable>(20, 30, 40, 50),
        std::make_shared<mtd::FakeRenderable>(0, 0, 1920, 1200)
    };

    EXPECT_EQ(list.end(), std::find_if(list.begin(), list.end(), matcher));
}

TEST_F(BypassFilterTest, translucently_obscured_fullscreen_window_not_bypassed)
{   // Regression test for LP: #1266385
    mc::BypassMatcher matcher(primary_monitor);

    mg::RenderableList list{
        std::make_shared<mtd::FakeRenderable>(20, 30, 40, 50, 0.5f),
        std::make_shared<mtd::FakeRenderable>(0, 0, 1920, 1200)
    };

    EXPECT_EQ(list.end(), std::find_if(list.begin(), list.end(), matcher));
}

TEST_F(BypassFilterTest, unobscured_fullscreen_window_bypassed)
{
    mc::BypassMatcher matcher(primary_monitor);

    auto bypassed = std::make_shared<mtd::FakeRenderable>(0, 0, 1920, 1200);
    mg::RenderableList list{
        bypassed,
        std::make_shared<mtd::FakeRenderable>(20, 30, 40, 50)
    };

    auto it = std::find_if(list.begin(), list.end(), matcher);
    EXPECT_NE(list.end(), it);
    EXPECT_EQ(bypassed, *it);
}

TEST_F(BypassFilterTest, unobscured_fullscreen_alpha_window_not_bypassed)
{
    mc::BypassMatcher matcher(primary_monitor);

    mg::RenderableList list{
        std::make_shared<mtd::FakeRenderable>(0, 0, 1920, 1200, 0.9f),
        std::make_shared<mtd::FakeRenderable>(20, 30, 40, 50)
    };

    EXPECT_EQ(list.end(), std::find_if(list.begin(), list.end(), matcher));
}

TEST_F(BypassFilterTest, many_fullscreen_windows_only_bypass_top)
{
    mc::BypassMatcher matcher(primary_monitor);

    auto bypassed = std::make_shared<mtd::FakeRenderable>(0, 0, 1920, 1200);
    auto fullscreen_not_bypassed = std::make_shared<mtd::FakeRenderable>(0, 0, 1920, 1200);
    mg::RenderableList list{
        bypassed,
        std::make_shared<mtd::FakeRenderable>(1, 2, 3, 4),
        fullscreen_not_bypassed,
        std::make_shared<mtd::FakeRenderable>(5, 6, 7, 8),
        fullscreen_not_bypassed,
        std::make_shared<mtd::FakeRenderable>(9, 10, 11, 12),
        fullscreen_not_bypassed
    };

    auto it = std::find_if(list.begin(), list.end(), matcher);
    EXPECT_NE(list.end(), it);
    EXPECT_EQ(bypassed, *it);
}

TEST_F(BypassFilterTest, many_fullscreen_windows_only_bypass_top_rectangular)
{
    mc::BypassMatcher matcher(primary_monitor);

    auto bypassed = std::make_shared<mtd::FakeRenderable>(0, 0, 1920, 1200);
    mg::RenderableList list{
        bypassed,
        std::make_shared<mtd::FakeRenderable>(0, 0, 1920, 1200, 1.0f, false),
        std::make_shared<mtd::FakeRenderable>(1, 2, 3, 4),
        std::make_shared<mtd::FakeRenderable>(0, 0, 1920, 1200),
        std::make_shared<mtd::FakeRenderable>(5, 6, 7, 8),
        std::make_shared<mtd::FakeRenderable>(0, 0, 1920, 1200, 1.0f, true),
        std::make_shared<mtd::FakeRenderable>(9, 10, 11, 12),
        std::make_shared<mtd::FakeRenderable>(0, 0, 1920, 1200, 0.5f, false)
    };

    auto it = std::find_if(list.begin(), list.end(), matcher);
    EXPECT_NE(list.end(), it);
    EXPECT_EQ(bypassed, *it);
}

TEST_F(BypassFilterTest, nonrectangular_not_bypassable)
{
    mc::BypassMatcher matcher(primary_monitor);

    auto bypassed = std::make_shared<mtd::FakeRenderable>(0, 0, 1920, 1200);
    auto fullscreen_not_bypassed = std::make_shared<mtd::FakeRenderable>(0, 0, 1920, 1200);
    mg::RenderableList list{
        std::make_shared<mtd::FakeRenderable>(0, 0, 1920, 1200, 1.0f, false),
        std::make_shared<mtd::FakeRenderable>(1, 2, 3, 4)
    };

    EXPECT_EQ(list.end(), std::find_if(list.begin(), list.end(), matcher));
}

TEST_F(BypassFilterTest, nonvisible_not_bypassble)
{
    mc::BypassMatcher matcher(primary_monitor);
    mg::RenderableList list{
        std::make_shared<mtd::FakeRenderable>(0, 0, 1920, 1200, 1.0f, true, false, true)
    };

    EXPECT_EQ(list.end(), std::find_if(list.begin(), list.end(), matcher));
}

TEST_F(BypassFilterTest, offscreen_not_bypassable)
{
    mc::BypassMatcher matcher(primary_monitor);
    mg::RenderableList list{
        std::make_shared<mtd::FakeRenderable>(0, 0, 1920, 1200, 1.0f, true, true, false)
    };
    EXPECT_EQ(list.end(), std::find_if(list.begin(), list.end(), matcher));
}

TEST_F(BypassFilterTest, multimonitor_one_bypassed)
{
    mc::BypassMatcher primary_matcher(primary_monitor);
    mc::BypassMatcher secondary_matcher(primary_monitor);

    auto bypassed = std::make_shared<mtd::FakeRenderable>(1920, 0, 1920, 1200);
    mg::RenderableList list{
        bypassed,
        std::make_shared<mtd::FakeRenderable>(20, 30, 40, 50)
    };

    EXPECT_EQ(list.end(), std::find_if(list.begin(), list.end(), primary_matcher));

    auto it = std::find_if(list.begin(), list.end(), secondary_matcher);
    EXPECT_NE(list.end(), it);
    EXPECT_EQ(bypassed, *it);
}

TEST_F(BypassFilterTest, dual_bypass)
{
    mc::BypassMatcher primary_matcher(primary_monitor);
    mc::BypassMatcher secondary_matcher(primary_monitor);

    auto primary_bypassed = std::make_shared<mtd::FakeRenderable>(0, 0, 1920, 1200);
    auto secondary_bypassed = std::make_shared<mtd::FakeRenderable>(1920, 0, 1920, 1200);
    mg::RenderableList list{
        primary_bypassed,
        secondary_bypassed
    };

    auto it = std::find_if(list.begin(), list.end(), primary_matcher);
    EXPECT_NE(list.end(), it);
    EXPECT_EQ(primary_bypassed, *it);

    it = std::find_if(list.begin(), list.end(), secondary_matcher);
    EXPECT_NE(list.end(), it);
    EXPECT_EQ(secondary_bypassed, *it);
}

TEST_F(BypassFilterTest, multimonitor_oversized_no_bypass)
{
    mc::BypassMatcher primary_matcher(primary_monitor);
    mc::BypassMatcher secondary_matcher(primary_monitor);

    mg::RenderableList list{
        std::make_shared<mtd::FakeRenderable>(0, 0, 3840, 1200)
    };

    EXPECT_EQ(list.end(), std::find_if(list.begin(), list.end(), primary_matcher));
    EXPECT_EQ(list.end(), std::find_if(list.begin(), list.end(), secondary_matcher));
}

#if 0
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
