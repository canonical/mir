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
#include "examples/demo-shell/demo_compositor.h"
#include <gtest/gtest.h>

#include <iostream>

namespace mtd = mir::test::doubles;
namespace mg = mir::graphics;
namespace geom = mir::geometry;
namespace me = mir::examples;

TEST(DemoShadowDetection, detects_shadows_in_list)
{
    int const shadow_radius{20};
    int const titlebar_height{5};
    geom::Rectangle region{{0, 0}, {100, 100}};

    mg::RenderableList fullscreen_surface
        { std::make_shared<mtd::FakeRenderable>(region) };
    mg::RenderableList oversized_surface
        { std::make_shared<mtd::FakeRenderable>(geom::Rectangle{{-10, -10}, {120, 120}}) };
    mg::RenderableList top_shadow
        { std::make_shared<mtd::FakeRenderable>(geom::Rectangle{{0, 105}, {10, 10}}) };
    mg::RenderableList right_shadow
        { std::make_shared<mtd::FakeRenderable>(geom::Rectangle{{-10, 0}, {10, 10}}) };
    mg::RenderableList left_shadow
        { std::make_shared<mtd::FakeRenderable>(geom::Rectangle{{100, 0}, {10, 10}}) };
    mg::RenderableList bottom_shadow
        { std::make_shared<mtd::FakeRenderable>(geom::Rectangle{{-10, 0}, {10, 10}}) };
    mg::RenderableList only_titlebar
        { std::make_shared<mtd::FakeRenderable>(geom::Rectangle{{0, 5}, {100, 95}}) };

    EXPECT_TRUE(me::shadows_or_titlebar_in_region(top_shadow, region, shadow_radius, titlebar_height));
    EXPECT_TRUE(me::shadows_or_titlebar_in_region(right_shadow, region, shadow_radius, titlebar_height));
    EXPECT_TRUE(me::shadows_or_titlebar_in_region(bottom_shadow, region, shadow_radius, titlebar_height));
    EXPECT_TRUE(me::shadows_or_titlebar_in_region(left_shadow, region, shadow_radius, titlebar_height));
    EXPECT_TRUE(me::shadows_or_titlebar_in_region(only_titlebar, region, titlebar_height));

    EXPECT_FALSE(me::shadows_or_titlebar_in_region(fullscreen_surface, region, shadow_radius, titlebar_height));
    EXPECT_FALSE(me::shadows_or_titlebar_in_region(oversized_surface, region, shadow_radius, titlebar_height));
}
