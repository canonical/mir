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

namespace
{
class StubRenderer : public mg::Renderer
{
public:
    StubRenderer()
     : resource0 (std::make_shared<int>(43)),
       resource1 (std::make_shared<int>(42)),
       resource2 (std::make_shared<int>(41)),
       counter(0)
    {

    }
    void render(std::function<void(std::shared_ptr<void> const&)> save_resource, mg::Renderable&)
    {
        std::shared_ptr<void> tmp;
        switch(counter++)
        {
            case 0:
                tmp = resource0;
                save_resource(tmp);
                break;
            case 1:
                tmp = resource1;
                save_resource(tmp);
                break;
            case 2:
                tmp = resource2;
                save_resource(tmp);
                break;
            default:
                FAIL() << "this stub renderer can only have render() called 3 times";
                break;
        }
    }

    std::shared_ptr<int> resource0;
    std::shared_ptr<int> resource1;
    std::shared_ptr<int> resource2;
    int counter;
};
}

TEST(RenderingOperator, render_operator_holds_resources_over_its_lifetime)
{
    using namespace testing;

    StubRenderer stub_renderer;
    mtd::MockRenderable mock_renderable;

    auto use_count_before0 = stub_renderer.resource0.use_count();
    auto use_count_before1 = stub_renderer.resource1.use_count();
    auto use_count_before2 = stub_renderer.resource2.use_count();
    {
        mc::RenderingOperator rendering_operator(stub_renderer);

        rendering_operator(mock_renderable);
        rendering_operator(mock_renderable);
        rendering_operator(mock_renderable);
        EXPECT_EQ(use_count_before0 + 1, stub_renderer.resource0.use_count());
        EXPECT_EQ(use_count_before1 + 1, stub_renderer.resource1.use_count());
        EXPECT_EQ(use_count_before2 + 1, stub_renderer.resource2.use_count());
    }

    EXPECT_EQ(use_count_before0, stub_renderer.resource0.use_count());
    EXPECT_EQ(use_count_before1, stub_renderer.resource1.use_count());
    EXPECT_EQ(use_count_before2, stub_renderer.resource2.use_count());
}
