/*
 * Copyright © 2013 Canonical Ltd.
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
#include "mir/shell/surface_creation_parameters.h"

#include "mir_test/fake_shared.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace msh = mir::shell;
namespace ms = mir::scene;
namespace mt = mir::test;

namespace
{
struct MockSurfaceStackModel : public ms::SurfaceStackModel
{
    MOCK_METHOD1(create_surface, std::weak_ptr<ms::BasicSurface>(msh::SurfaceCreationParameters const&));
    MOCK_METHOD1(destroy_surface, void(std::weak_ptr<ms::BasicSurface> const&));
    MOCK_METHOD1(raise, void(std::weak_ptr<ms::BasicSurface> const&));
};
}

TEST(SurfaceController, create_and_destroy_surface)
{
    using namespace ::testing;

    std::weak_ptr<ms::BasicSurface> null_surface;
    MockSurfaceStackModel model;

    ms::SurfaceController controller(mt::fake_shared(model));

    InSequence seq;
    EXPECT_CALL(model, create_surface(_)).Times(1).WillOnce(Return(null_surface));
    EXPECT_CALL(model, destroy_surface(_)).Times(1);

    auto surface = controller.create_surface(msh::a_surface());
    controller.destroy_surface(surface);
}

TEST(SurfaceController, raise_surface)
{
    using namespace ::testing;

    MockSurfaceStackModel model;
    ms::SurfaceController controller(mt::fake_shared(model));

    EXPECT_CALL(model, raise(_)).Times(1);

    controller.raise(std::weak_ptr<ms::BasicSurface>());
}
