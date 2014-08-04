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
#include "mir/compositor/destination_alpha.h"
#include "mir_test_doubles/fake_renderable.h"
#include "mir_test_doubles/stub_gl_program_factory.h"
#include "examples/demo-shell/demo_renderer.h"
#include <gtest/gtest.h>

namespace mtd = mir::test::doubles;
namespace mg = mir::graphics;
namespace geom = mir::geometry;
namespace me = mir::examples;
namespace mc = mir::compositor;

struct DemoRenderer : public testing::Test
{
    mtd::StubGLProgramFactory stub_factory;
    geom::Rectangle region{{0, 0}, {100, 100}};
    mc::DestinationAlpha dest_alpha{mc::DestinationAlpha::opaque};
    int const shadow_radius{20};
    int const titlebar_height{5};
};

TEST_F(DemoRenderer, detects_shadows_in_list)
{
    me::DemoRenderer demo_renderer(stub_factory, region, dest_alpha, titlebar_height, shadow_radius);

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

    EXPECT_TRUE(demo_renderer.would_render_decorations_on(top_shadow, region));
    EXPECT_TRUE(demo_renderer.would_render_decorations_on(right_shadow, region));
    EXPECT_TRUE(demo_renderer.would_render_decorations_on(bottom_shadow, region));
    EXPECT_TRUE(demo_renderer.would_render_decorations_on(left_shadow, region));
    EXPECT_TRUE(demo_renderer.would_render_decorations_on(only_titlebar, region));
    EXPECT_FALSE(demo_renderer.would_render_decorations_on(fullscreen_surface, region));
    EXPECT_FALSE(demo_renderer.would_render_decorations_on(oversized_surface, region));
}
