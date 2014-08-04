/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir/graphics/renderable.h"
#include "mir_test_doubles/fake_renderable.h"
#include <gtest/gtest.h>

#include <iostream>

namespace mtd = mir::test::doubles;
namespace mg = mir::graphics;
namespace geom = mir::geometry;

namespace
{
bool shadows_contained_in_region(
    mg::RenderableList const& renderables,
    geom::Rectangle const region,
    unsigned int shadow_radius)
{
    for(auto const& r : renderables)
    {
        auto const& window = r->screen_position();
        geom::Rectangle const shadow_right{
            window.top_right(),
            geom::Size{shadow_radius, window.size.height.as_int()}};
        geom::Rectangle const shadow_bottom{
            window.bottom_left(),
            geom::Size{window.size.width.as_int(), shadow_radius}};
        geom::Rectangle const shadow_corner{
            window.bottom_right(),
            geom::Size{shadow_radius, shadow_radius}};

        if (region.contains(shadow_right) ||
            region.contains(shadow_bottom) ||
            region.contains(shadow_corner))
            return true;
    }
    return false;
}

bool titlebar_contained_in_region(
    mg::RenderableList const& renderables,
    geom::Rectangle const region,
    unsigned int titlebar_height)
{
    for(auto const& r : renderables)
    {
        auto const& window = r->screen_position();
        geom::Rectangle const titlebar{
            geom::Point{(window.top_left.x.as_int() - titlebar_height), window.top_left.y},
            geom::Size{window.size.width.as_int(), titlebar_height}
        };
        if (region.contains(titlebar))
            return true;
    }
    return false; 
}
}

TEST(ShadowDetection, detects_shadows_in_list)
{
    int const shadow_radius{20};
    geom::Rectangle region{{0, 0}, {100, 100}};

    mg::RenderableList shadow_present 
    {
        std::make_shared<mtd::FakeRenderable>(geom::Rectangle{{10, 10}, {10, 10}})
    };
    mg::RenderableList shadow_not_present
    {
        std::make_shared<mtd::FakeRenderable>(geom::Rectangle{{90, 90}, {10, 10}})
    };
    mg::RenderableList oversized_surface
    {
        std::make_shared<mtd::FakeRenderable>(geom::Rectangle{{-10, -10}, {120, 120}})
    };
    mg::RenderableList right_shadow
    {
        std::make_shared<mtd::FakeRenderable>(geom::Rectangle{{10, 90}, {10, 10}})
    };
    mg::RenderableList bottom_shadow
    {
        std::make_shared<mtd::FakeRenderable>(geom::Rectangle{{90, 10}, {10, 10}})
    };
    mg::RenderableList corner_shadow
    {
        std::make_shared<mtd::FakeRenderable>(geom::Rectangle{{-8, -8}, {10, 10}})
    };

    EXPECT_TRUE(shadows_contained_in_region(shadow_present, region, shadow_radius));
    EXPECT_TRUE(shadows_contained_in_region(right_shadow, region, shadow_radius));
    EXPECT_TRUE(shadows_contained_in_region(bottom_shadow, region, shadow_radius));
    EXPECT_TRUE(shadows_contained_in_region(corner_shadow, region, shadow_radius));
    EXPECT_FALSE(shadows_contained_in_region(shadow_not_present, region, shadow_radius));
    EXPECT_FALSE(shadows_contained_in_region(oversized_surface, region, shadow_radius));
}

TEST(ShadowDetection, detects_titlebar_in_list)
{
    geom::Rectangle region{{0, 0}, {100, 100}};
    int const titlebar_height{5};
    mg::RenderableList titlebar
    {
        std::make_shared<mtd::FakeRenderable>(geom::Rectangle{{10, 10}, {10, 10}})
    };
    mg::RenderableList offscreen_titlebar
    {
        std::make_shared<mtd::FakeRenderable>(geom::Rectangle{{0, 0}, {100, 100}})
    };

    EXPECT_TRUE(titlebar_contained_in_region(titlebar, region, titlebar_height));
    EXPECT_FALSE(titlebar_contained_in_region(offscreen_titlebar, region, titlebar_height));
    
}
