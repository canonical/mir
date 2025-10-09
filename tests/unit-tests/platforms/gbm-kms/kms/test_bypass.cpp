/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "src/platforms/gbm-kms/server/kms/bypass.h"
#include "mir/test/doubles/fake_renderable.h"

#include <gtest/gtest.h>
#include <memory>

namespace mg=mir::graphics;
namespace geom=mir::geometry;
namespace mgg=mir::graphics::gbm;
namespace mtd=mir::test::doubles;

struct BypassMatchTest : public testing::Test
{
    geom::Rectangle const primary_monitor{{0, 0},{1920, 1200}};
    geom::Rectangle const secondary_monitor{{1920, 0},{1920, 1200}};
};

TEST_F(BypassMatchTest, nothing_matches_nothing)
{
    mg::RenderableList empty_list{};
    mgg::BypassMatch matcher(primary_monitor);

    EXPECT_EQ(empty_list.rend(), std::find_if(empty_list.rbegin(), empty_list.rend(), matcher));
}

TEST_F(BypassMatchTest, small_window_not_bypassed)
{
    mgg::BypassMatch matcher(primary_monitor);
    mg::RenderableList list{
        std::make_shared<mtd::FakeRenderable>(12, 34, 56, 78)
    };

    EXPECT_EQ(list.rend(), std::find_if(list.rbegin(), list.rend(), matcher));
}

TEST_F(BypassMatchTest, single_fullscreen_window_bypassed)
{
    auto window = std::make_shared<mtd::FakeRenderable>(0, 0, 1920, 1200);
    mgg::BypassMatch matcher(primary_monitor);
    mg::RenderableList list{window};

    auto it = std::find_if(list.rbegin(), list.rend(), matcher);
    EXPECT_NE(list.rend(), it);
    EXPECT_EQ(window, *it);
}

TEST_F(BypassMatchTest, translucent_fullscreen_window_not_bypassed)
{
    mgg::BypassMatch matcher(primary_monitor);
    mg::RenderableList list{
        std::make_shared<mtd::FakeRenderable>(geom::Rectangle{{0, 0}, {1920, 1200}}, 0.5f)
    };

    EXPECT_EQ(list.rend(), std::find_if(list.rbegin(), list.rend(), matcher));
}

TEST_F(BypassMatchTest, shaped_fullscreen_window_not_bypassed)
{
    mgg::BypassMatch matcher(primary_monitor);

    mg::RenderableList list{
        std::make_shared<mtd::FakeRenderable>(geom::Rectangle{{0, 0}, {1920, 1200}}, 1.0f, false, std::nullopt)
    };

    EXPECT_EQ(list.rend(), std::find_if(list.rbegin(), list.rend(), matcher));
}

TEST_F(BypassMatchTest, offset_fullscreen_window_not_bypassed)
{
    mgg::BypassMatch matcher(primary_monitor);

    mg::RenderableList list{
        std::make_shared<mtd::FakeRenderable>(10, 50, 1920, 1200)
    };

    EXPECT_EQ(list.rend(), std::find_if(list.rbegin(), list.rend(), matcher));
}

TEST_F(BypassMatchTest, obscured_fullscreen_window_not_bypassed)
{
    mgg::BypassMatch matcher(primary_monitor);

    mg::RenderableList list{
        std::make_shared<mtd::FakeRenderable>(0, 0, 1920, 1200),
        std::make_shared<mtd::FakeRenderable>(20, 30, 40, 50)
    };

    EXPECT_EQ(list.rend(), std::find_if(list.rbegin(), list.rend(), matcher));
}

TEST_F(BypassMatchTest, translucently_obscured_fullscreen_window_not_bypassed)
{   // Regression test for LP: #1266385
    mgg::BypassMatch matcher(primary_monitor);

    mg::RenderableList list{
        std::make_shared<mtd::FakeRenderable>(0, 0, 1920, 1200),
        std::make_shared<mtd::FakeRenderable>(geom::Rectangle{{20, 30}, {40, 50}}, 0.5f)
    };

    EXPECT_EQ(list.rend(), std::find_if(list.rbegin(), list.rend(), matcher));
}

TEST_F(BypassMatchTest, unobscured_fullscreen_window_bypassed)
{
    mgg::BypassMatch matcher(primary_monitor);

    auto bypassed = std::make_shared<mtd::FakeRenderable>(0, 0, 1920, 1200);
    mg::RenderableList list{
        std::make_shared<mtd::FakeRenderable>(20, 30, 40, 50),
        bypassed
    };

    auto it = std::find_if(list.rbegin(), list.rend(), matcher);
    EXPECT_NE(list.rend(), it);
    EXPECT_EQ(bypassed, *it);
}

TEST_F(BypassMatchTest, unobscured_fullscreen_alpha_window_not_bypassed)
{
    mgg::BypassMatch matcher(primary_monitor);

    mg::RenderableList list{
        std::make_shared<mtd::FakeRenderable>(20, 30, 40, 50),
        std::make_shared<mtd::FakeRenderable>(geom::Rectangle{{0, 0}, {1920, 1200}}, 0.9f)
    };

    EXPECT_EQ(list.rend(), std::find_if(list.rbegin(), list.rend(), matcher));
}

TEST_F(BypassMatchTest, many_fullscreen_windows_only_bypass_top)
{
    mgg::BypassMatch matcher(primary_monitor);

    auto bypassed = std::make_shared<mtd::FakeRenderable>(0, 0, 1920, 1200);
    auto fullscreen_not_bypassed = std::make_shared<mtd::FakeRenderable>(0, 0, 1920, 1200);
    mg::RenderableList list{
        fullscreen_not_bypassed,
        std::make_shared<mtd::FakeRenderable>(9, 10, 11, 12),
        fullscreen_not_bypassed,
        std::make_shared<mtd::FakeRenderable>(5, 6, 7, 8),
        fullscreen_not_bypassed,
        std::make_shared<mtd::FakeRenderable>(1, 2, 3, 4),
        bypassed
    };

    auto it = std::find_if(list.rbegin(), list.rend(), matcher);
    EXPECT_NE(list.rend(), it);
    EXPECT_EQ(bypassed, *it);
}

TEST_F(BypassMatchTest, detects_window_that_intersects_full_screen_window)
{
    mgg::BypassMatch matcher(primary_monitor);

    auto bypassable = std::make_shared<mtd::FakeRenderable>(0, 0, 1920, 1200);
    auto small_intersection_window = std::make_shared<mtd::FakeRenderable>(1910, 600, 20, 20);
    mg::RenderableList list{
        bypassable,
        small_intersection_window
    };

    auto it = std::find_if(list.rbegin(), list.rend(), matcher);
    EXPECT_EQ(list.rend(), it);
}

TEST_F(BypassMatchTest, many_fullscreen_windows_only_bypass_top_rectangular)
{
    mgg::BypassMatch matcher(primary_monitor);

    auto bypassed = std::make_shared<mtd::FakeRenderable>(0, 0, 1920, 1200);
    mg::RenderableList list{
        std::make_shared<mtd::FakeRenderable>(primary_monitor, 0.5f, false, std::nullopt),
        std::make_shared<mtd::FakeRenderable>(geom::Rectangle{{9, 10}, {11, 12}}),
        std::make_shared<mtd::FakeRenderable>(primary_monitor, 1.0f, true, std::nullopt),
        std::make_shared<mtd::FakeRenderable>(geom::Rectangle{{5, 6}, {7, 8}}),
        std::make_shared<mtd::FakeRenderable>(primary_monitor),
        std::make_shared<mtd::FakeRenderable>(geom::Rectangle{{1, 2}, {3, 4}}),
        std::make_shared<mtd::FakeRenderable>(primary_monitor, 1.0f, false, std::nullopt),
        bypassed
    };

    auto it = std::find_if(list.rbegin(), list.rend(), matcher);
    EXPECT_NE(list.rend(), it);
    EXPECT_EQ(bypassed, *it);
}

TEST_F(BypassMatchTest, nonrectangular_not_bypassable)
{
    mgg::BypassMatch matcher(primary_monitor);

    auto bypassed = std::make_shared<mtd::FakeRenderable>(0, 0, 1920, 1200);
    auto fullscreen_not_bypassed = std::make_shared<mtd::FakeRenderable>(0, 0, 1920, 1200);
    mg::RenderableList list{
        std::make_shared<mtd::FakeRenderable>(1, 2, 3, 4),
        std::make_shared<mtd::FakeRenderable>(primary_monitor, 1.0f, false, std::nullopt)
    };

    EXPECT_EQ(list.rend(), std::find_if(list.rbegin(), list.rend(), matcher));
}

TEST_F(BypassMatchTest, multimonitor_one_bypassed)
{
    mgg::BypassMatch primary_matcher(primary_monitor);
    mgg::BypassMatch secondary_matcher(secondary_monitor);

    auto bypassed = std::make_shared<mtd::FakeRenderable>(1920, 0, 1920, 1200);
    mg::RenderableList list{
        std::make_shared<mtd::FakeRenderable>(20, 30, 40, 50),
        bypassed
    };

    EXPECT_EQ(list.rend(), std::find_if(list.rbegin(), list.rend(), primary_matcher));

    auto it = std::find_if(list.rbegin(), list.rend(), secondary_matcher);
    EXPECT_NE(list.rend(), it);
    EXPECT_EQ(bypassed, *it);
}

TEST_F(BypassMatchTest, dual_bypass)
{
    mgg::BypassMatch primary_matcher(primary_monitor);
    mgg::BypassMatch secondary_matcher(secondary_monitor);

    auto primary_bypassed = std::make_shared<mtd::FakeRenderable>(0, 0, 1920, 1200);
    auto secondary_bypassed = std::make_shared<mtd::FakeRenderable>(1920, 0, 1920, 1200);
    mg::RenderableList list{
        primary_bypassed,
        secondary_bypassed
    };

    auto it = std::find_if(list.rbegin(), list.rend(), primary_matcher);
    EXPECT_NE(list.rend(), it);
    EXPECT_EQ(primary_bypassed, *it);

    it = std::find_if(list.rbegin(), list.rend(), secondary_matcher);
    EXPECT_NE(list.rend(), it);
    EXPECT_EQ(secondary_bypassed, *it);
}

TEST_F(BypassMatchTest, multimonitor_oversized_no_bypass)
{
    mgg::BypassMatch primary_matcher(primary_monitor);
    mgg::BypassMatch secondary_matcher(secondary_monitor);

    mg::RenderableList list{
        std::make_shared<mtd::FakeRenderable>(0, 0, 3840, 1200)
    };

    EXPECT_EQ(list.rend(), std::find_if(list.rbegin(), list.rend(), primary_matcher));
    EXPECT_EQ(list.rend(), std::find_if(list.rbegin(), list.rend(), secondary_matcher));
}
