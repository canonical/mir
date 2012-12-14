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

#include "mir/compositor/rendering_operator_for_renderables.h"
#include "mir_test_doubles/mock_surface_renderer.h"
#include "mir_test_doubles/mock_graphic_region.h"
#include "mir_test_doubles/mock_renderable.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "mir_test/gmock_fixes.h"

namespace mc = mir::compositor;
namespace mg = mir::graphics;
namespace geom = mir::geometry;
namespace mtd = mir::test::doubles;
#if 0
TEST(RenderingOperator,
    render_operator_hold_resource)
{
    using namespace testing;

    NiceMock<mtd::MockSurfaceRenderer> mock_renderer;
    mtd::MockRenderable mock_renderable_a;
    mtd::MockRenderable mock_renderable_b;
    mtd::MockRenderable mock_renderable_c;

    auto resource_a = std::make_shared<mtd::MockGraphicRegion>();
    auto resource_b = std::make_shared<mtd::MockGraphicRegion>();
    auto resource_c = std::make_shared<mtd::MockGraphicRegion>();

    long use_count_a_before, use_count_b_before, use_count_c_before;
    {
        mc::RenderingOperatorForRenderables rendering_operator(mock_renderer);

        EXPECT_CALL(mock_renderable_a, texture())
            .Times(1)
            .WillOnce(Return(resource_a));
        EXPECT_CALL(mock_renderable_b, texture())
            .Times(1)
            .WillOnce(Return(resource_b));
        EXPECT_CALL(mock_renderable_c, texture())
            .Times(1)
            .WillOnce(Return(resource_c));

        use_count_a_before = resource_a.use_count();
        use_count_b_before = resource_b.use_count();
        use_count_c_before = resource_c.use_count();

        rendering_operator(mock_renderable_a);
        rendering_operator(mock_renderable_b);
        rendering_operator(mock_renderable_c);

        EXPECT_GT(resource_a.use_count(), use_count_a_before);
        EXPECT_GT(resource_b.use_count(), use_count_b_before);
        EXPECT_GT(resource_c.use_count(), use_count_c_before);
    }

    EXPECT_EQ(resource_a.use_count(), use_count_a_before);
    EXPECT_EQ(resource_b.use_count(), use_count_b_before);
    EXPECT_EQ(resource_c.use_count(), use_count_c_before);
}
#endif
TEST(RenderingOperator,
    render_operator_submits_resource_it_saves_to_renderer)
{
    using namespace testing;

    mtd::MockSurfaceRenderer mock_renderer;
    mtd::MockRenderable mock_renderable;
    auto resource = std::make_shared<mc::GraphicBufferCompositorResource>();
    auto region = std::make_shared<mtd::MockGraphicRegion>();
    resource->region = region;

    mc::RenderingOperatorForRenderables rendering_operator(mock_renderer);

    EXPECT_CALL(mock_renderable, texture())
        .Times(1)
        .WillOnce(Return(resource));
    EXPECT_CALL(mock_renderer, render(_, resource->region.lock()))
        .Times(1);

    rendering_operator(mock_renderable);
}
