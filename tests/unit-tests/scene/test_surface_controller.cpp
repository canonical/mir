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
#include "src/server/scene/basic_surface_factory.h"
#include "mir/shell/surface_creation_parameters.h"

#include "mir_test/fake_shared.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mf = mir::frontend;
namespace msh = mir::shell;
namespace ms = mir::scene;
namespace mt = mir::test;

namespace
{
struct MockSurfaceAllocator : public ms::BasicSurfaceFactory
{
    MOCK_METHOD3(create_surface, std::shared_ptr<ms::Surface>(
        mf::SurfaceId id,
        msh::SurfaceCreationParameters const&,
        std::shared_ptr<mf::EventSink> const&));
};

struct MockSurfaceStackModel : public ms::SurfaceStackModel
{
    MOCK_METHOD3(add_surface, void(
        std::shared_ptr<ms::Surface> const&,
        ms::DepthId depth,
        mir::input::InputReceptionMode input_mode));
    MOCK_METHOD1(remove_surface, void(std::weak_ptr<ms::Surface> const&));
    MOCK_METHOD1(raise, void(std::weak_ptr<ms::Surface> const&));
};
}

TEST(SurfaceController, create_and_destroy_surface)
{
    using namespace ::testing;

    std::shared_ptr<ms::Surface> null_surface;
    testing::NiceMock<MockSurfaceAllocator> mock_surface_allocator;
    MockSurfaceStackModel model;

    ms::SurfaceController controller(mt::fake_shared(mock_surface_allocator), mt::fake_shared(model));

    InSequence seq;
    EXPECT_CALL(mock_surface_allocator, create_surface(_,_,_)).Times(1).WillOnce(Return(null_surface));
    EXPECT_CALL(model, add_surface(_,_,_)).Times(1);
    EXPECT_CALL(model, remove_surface(_)).Times(1);

    auto surface = controller.create_surface(mf::SurfaceId(), msh::a_surface(), {});
    controller.destroy_surface(surface);
}

TEST(SurfaceController, raise_surface)
{
    using namespace ::testing;

    testing::NiceMock<MockSurfaceAllocator> mock_surface_allocator;
    MockSurfaceStackModel model;

    ms::SurfaceController controller(mt::fake_shared(mock_surface_allocator), mt::fake_shared(model));

    EXPECT_CALL(model, raise(_)).Times(1);

    controller.raise(std::weak_ptr<ms::Surface>());
}
