/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
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

#include "miral/minimal_window_manager.h"
#include "mir/geometry/rectangle.h"

#include <gtest/gtest.h>

using namespace testing;
namespace geom = mir::geometry;

struct WindowPlacementOcclusionChecks : testing::Test
{
};

TEST(WindowPlacementOcclusionChecks, is_not_occluded_with_no_occluding_windows)
{
    auto test_rect = geom::Rectangle{{100, 100}, {480, 360}};
    ASSERT_FALSE(miral::is_occluded(test_rect, {}, test_rect.size));
}

TEST(WindowPlacementOcclusionChecks, is_occluded_when_stacked)
{
    auto test_rect = geom::Rectangle{{100, 100}, {480, 360}};
    ASSERT_TRUE(miral::is_occluded(test_rect, {test_rect}, {0, 0}));
}

TEST(WindowPlacementOcclusionChecks, is_occluded_when_completely_contained)
{
    auto test_rect = geom::Rectangle{{100, 100}, {480, 360}};
    auto containing_rect = geom::Rectangle{{50, 50}, {580, 460}};
    ASSERT_TRUE(miral::is_occluded(test_rect, {containing_rect}, {0, 0}));
}

TEST(WindowPlacementOcclusionChecks, is_occluded_with_multiple_windows)
{
    auto test_rect = geom::Rectangle{{100, 100}, {480, 360}};

    //  Let's have 4 smaller rectangles occluding our test rectangle. Note that
    //  the smaller rectangles slightly overextend beyond the test one.
    //  ┌─────────────────────────┌──────────────────────────┐
    //  │                         │                          │
    //  │                         │                          │
    //  │    ┌────────────────────│─────────────────────┐    │
    //  │    │                    │                     │    │
    //  │    │                    │                     │    │
    //  │    │                    │                     │    │
    //  │    │                    │                     │    │
    //  │    │                    │                     │    │
    //  │    │                    │                     │    │
    //  │    │                    │                     │    │
    //  ┌─────────────────────────└──────────────────────────┘
    //  │    │                    │                     │    │
    //  │    │                    │                     │    │
    //  │    │                    │                     │    │
    //  │    │                    │                     │    │
    //  │    │                    │                     │    │
    //  │    │                    │                     │    │
    //  │    │                    │                     │    │
    //  │    └────────────────────│─────────────────────┘    │
    //  │                         │                          │
    //  │                         │                          │
    //  └─────────────────────────└──────────────────────────┘

    auto occluding_rects = {
        geom::Rectangle{{50, 50}, {290, 230}},
        geom::Rectangle{{340, 50}, {290, 230}},
        geom::Rectangle{{50, 280}, {290, 230}},
        geom::Rectangle{{340, 280}, {290, 230}},
    };
    ASSERT_TRUE(miral::is_occluded(test_rect, occluding_rects, {0, 0}));
}

TEST(WindowPlacementOcclusionChecks, is_not_occluded_with_multiple_windows_with_thin_gaps)
{
    auto test_rect = geom::Rectangle{{100, 100}, {480, 360}};

    // Like the last test case, but let's make a 20 pixel wide gap in the
    // center spanning the entire height
    auto occluding_rects = {
        geom::Rectangle{{50, 50}, {280, 230}},
        geom::Rectangle{{350, 50}, {290, 230}},
        geom::Rectangle{{50, 280}, {280, 230}},
        geom::Rectangle{{350, 280}, {290, 230}},
    };

    // Note that we get back two areas 20 pixels wide and 180 pixels high, not
    // one area 20x360. To get around this, we'd need to add a merging step
    ASSERT_TRUE(miral::is_occluded(test_rect, occluding_rects, {30, 100}));
    ASSERT_FALSE(miral::is_occluded(test_rect, occluding_rects, {20, 100}));
}

TEST(WindowPlacementOcclusionChecks, is_not_occluded_when_not_overlapping)
{
    auto test_rect = geom::Rectangle{{100, 100}, {480, 360}};

    auto occluding_rects = {
        geom::Rectangle{{-30, -100}, {100, 150}},
        geom::Rectangle{{600, 100}, {290, 230}},
    };

    ASSERT_FALSE(miral::is_occluded(test_rect, occluding_rects, test_rect.size));
}
