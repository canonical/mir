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
 * Authored by: Thomas Voss <thomas.voss@canonical.com>
 */

#include "mir/frontend/application_manager.h"
#include "mir/surfaces/surface_controller.h"
#include "mir/surfaces/surface_stack.h"
#include "mir/frontend/services/surface_factory.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mf = mir::frontend;
namespace mfs = mir::frontend::services;
namespace ms = mir::surfaces;

namespace
{

struct MockSurfaceStack : public ms::SurfaceStack
{
    MOCK_METHOD1(add_surface, void(std::weak_ptr<ms::Surface>));
    MOCK_METHOD1(remove_surface, void(std::weak_ptr<ms::Surface>));
};

}

TEST(TestApplicationManager, create_surface_adds_surface_to_surface_stack_via_surface_controller)
{
    using namespace ::testing;

    MockSurfaceStack surface_stack;
    ms::SurfaceController controller(&surface_stack);
    mf::ApplicationManager app_manager(&controller);

    EXPECT_CALL(surface_stack, add_surface(_)).Times(AtLeast(1));
    EXPECT_CALL(surface_stack, remove_surface(_)).Times(AtLeast(1));
                
    mfs::SurfaceFactory* surface_factory = &app_manager;
    std::shared_ptr<ms::Surface> surface = surface_factory->create_surface();

    ASSERT_TRUE(surface.get() != nullptr);

    surface_factory->destroy_surface(surface);
}
