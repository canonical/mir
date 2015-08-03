/*
 * Copyright © 2013-2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#include "src/server/scene/surface_controller.h"
#include "src/server/scene/surface_stack_model.h"
#include "mir/scene/surface_factory.h"
#include "mir/scene/surface_creation_parameters.h"
#include "mir/test/doubles/stub_scene_session.h"

#include "mir/test/doubles/mock_surface.h"
#include "mir/test/fake_shared.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace geom = mir::geometry;
namespace mf = mir::frontend;
namespace msh = mir::shell;
namespace ms = mir::scene;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;

namespace
{
struct MockSurfaceAllocator : public ms::SurfaceFactory
{
    MOCK_METHOD2(create_surface, std::shared_ptr<ms::Surface>(
        std::shared_ptr<mir::compositor::BufferStream> const&, ms::SurfaceCreationParameters const&));
};

struct MockSurfaceStackModel : public ms::SurfaceStackModel
{
    MOCK_METHOD3(add_surface, void(
        std::shared_ptr<ms::Surface> const&,
        ms::DepthId depth,
        mir::input::InputReceptionMode));
    MOCK_METHOD1(remove_surface, void(std::weak_ptr<ms::Surface> const&));
    MOCK_METHOD1(raise, void(std::weak_ptr<ms::Surface> const&));
    MOCK_CONST_METHOD1(surface_at, std::shared_ptr<ms::Surface>(geom::Point));
    void raise(SurfaceSet const&) override {}
};

struct SurfaceController : testing::Test
{
    mtd::MockSurface mock_surface;
    std::shared_ptr<ms::Surface> const expect_surface = mt::fake_shared(mock_surface);
    testing::NiceMock<MockSurfaceAllocator> mock_surface_allocator;
    MockSurfaceStackModel model;
    mtd::StubSceneSession session;

    void SetUp()
    {
        using namespace ::testing;
        ON_CALL(mock_surface_allocator, create_surface(_,_)).WillByDefault(Return(expect_surface));
    }
};
}

TEST_F(SurfaceController, add_and_remove_surface)
{
    using namespace ::testing;

    ms::SurfaceController controller(
        mt::fake_shared(mock_surface_allocator),
        mt::fake_shared(model));

    InSequence seq;
    EXPECT_CALL(model, add_surface(_,_,_)).Times(1);
    EXPECT_CALL(model, remove_surface(_)).Times(1);
    
    auto params = ms::a_surface();
    controller.add_surface(expect_surface, params.depth, params.input_mode, &session);
    controller.remove_surface(expect_surface);
}

TEST_F(SurfaceController, raise_surface)
{
    using namespace ::testing;

    ms::SurfaceController controller(
        mt::fake_shared(mock_surface_allocator),
        mt::fake_shared(model));

    EXPECT_CALL(model, raise(_)).Times(1);

    controller.raise(std::weak_ptr<ms::Surface>());
}

