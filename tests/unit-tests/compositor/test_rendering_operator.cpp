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

#include "mir/compositor/rendering_operator.h"
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

TEST(RenderingOperator, render_operator_holds_resources_over_its_lifetime)
{
    using namespace testing;

    mtd::MockSurfaceRenderer mock_renderer;
    mtd::MockRenderable mock_renderable;
    auto test_render_resource = std::make_shared<int>();

    mc::RenderingOperator rendering_operator(mock_renderer);

    EXPECT_CALL(mock_renderer, render( mock_renderable )
        .Times(1)
        .WillOnce(Return(test_render_resource));

    auto use_count_before = test_render_resource.count();
    {
        rendering_operator(mock_renderable);
        EXPECT_EQ(use_count_before, test_render_resource.count()+1);
    }

    EXPECT_EQ(use_count_before, test_render_resource.count()); 
}
