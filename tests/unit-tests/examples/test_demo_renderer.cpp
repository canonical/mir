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
#include "mir_test_doubles/mock_gl.h"
#include "playground/demo-shell/demo_renderer.h"
#include <gtest/gtest.h>

namespace mtd = mir::test::doubles;
namespace geom = mir::geometry;
namespace me = mir::examples;
namespace mc = mir::compositor;

struct DemoRenderer : public testing::Test
{
    testing::NiceMock<mtd::MockGL> mock_gl;
    mtd::StubGLProgramFactory stub_factory;
    geom::Rectangle region{{0, 0}, {100, 100}};
    mc::DestinationAlpha dest_alpha{mc::DestinationAlpha::opaque};
    int const shadow_radius{20};
    int const titlebar_height{5};
};

TEST_F(DemoRenderer, detects_embellishments_on_renderables)
{
    me::DemoRenderer demo_renderer(stub_factory, region, dest_alpha, titlebar_height, shadow_radius);

    mtd::FakeRenderable fullscreen_surface(region);
    mtd::FakeRenderable oversized_surface(geom::Rectangle{{-10, -10}, {120, 120}});
    mtd::FakeRenderable top_shadow(geom::Rectangle{{0, 105}, {10, 10}});
    mtd::FakeRenderable right_shadow(geom::Rectangle{{-10, 0}, {10, 10}});
    mtd::FakeRenderable left_shadow(geom::Rectangle{{100, 0}, {10, 10}});
    mtd::FakeRenderable bottom_shadow(geom::Rectangle{{-10, 0}, {10, 10}});
    mtd::FakeRenderable only_titlebar(geom::Rectangle{{0, 5}, {100, 95}});

    EXPECT_TRUE(demo_renderer.would_embellish(top_shadow, region));
    EXPECT_TRUE(demo_renderer.would_embellish(right_shadow, region));
    EXPECT_TRUE(demo_renderer.would_embellish(bottom_shadow, region));
    EXPECT_TRUE(demo_renderer.would_embellish(left_shadow, region));
    EXPECT_TRUE(demo_renderer.would_embellish(only_titlebar, region));
    EXPECT_FALSE(demo_renderer.would_embellish(fullscreen_surface, region));
    EXPECT_FALSE(demo_renderer.would_embellish(oversized_surface, region));
}
