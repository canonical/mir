/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored By: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir/shell/session_coordinator_wrapper.h"
#include "mir/shell/surface_coordinator_wrapper.h"

#include "mir_test_framework/headless_test.h"

#include "mir_test_framework/executable_path.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace ms = mir::scene;
namespace msh = mir::shell;
namespace mtf = mir_test_framework;

using namespace testing;

namespace
{
struct MySurfaceCoordinator : msh::SurfaceCoordinatorWrapper
{
    using msh::SurfaceCoordinatorWrapper::SurfaceCoordinatorWrapper;
    MOCK_METHOD1(raise, void(std::weak_ptr<ms::Surface> const&));
};

struct MySessionCoordinator : msh::SessionCoordinatorWrapper
{
    using msh::SessionCoordinatorWrapper::SessionCoordinatorWrapper;

    MOCK_METHOD0(focus_next, void());
};

struct ServerConfigurationWrapping : mir_test_framework::HeadlessTest
{
    void SetUp() override
    {
        server.wrap_surface_coordinator([]
            (std::shared_ptr<ms::SurfaceCoordinator> const& wrapped)
            {
                return std::make_shared<MySurfaceCoordinator>(wrapped);
            });

        server.wrap_session_coordinator([]
            (std::shared_ptr<ms::SessionCoordinator> const& wrapped)
            {
                return std::make_shared<MySessionCoordinator>(wrapped);
            });

        server.apply_settings();

        surface_coordinator = server.the_surface_coordinator();
        session_coordinator = server.the_session_coordinator();
    }

    std::shared_ptr<ms::SurfaceCoordinator> surface_coordinator;
    std::shared_ptr<ms::SessionCoordinator> session_coordinator;
};
}

TEST_F(ServerConfigurationWrapping, surface_coordinator_is_of_wrapper_type)
{
    auto const my_surface_coordinator = std::dynamic_pointer_cast<MySurfaceCoordinator>(surface_coordinator);

    EXPECT_THAT(my_surface_coordinator, Ne(nullptr));
}

TEST_F(ServerConfigurationWrapping, can_override_surface_coordinator_methods)
{
    auto const my_surface_coordinator = std::dynamic_pointer_cast<MySurfaceCoordinator>(surface_coordinator);

    EXPECT_CALL(*my_surface_coordinator, raise(_)).Times(1);
    surface_coordinator->raise({});
}

TEST_F(ServerConfigurationWrapping, returns_same_surface_coordinator_from_cache)
{
    ASSERT_THAT(server.the_surface_coordinator(), Eq(surface_coordinator));
}

TEST_F(ServerConfigurationWrapping, session_coordinator_is_of_wrapper_type)
{
    auto const my_session_coordinator = std::dynamic_pointer_cast<MySessionCoordinator>(session_coordinator);

    EXPECT_THAT(my_session_coordinator, Ne(nullptr));
}

TEST_F(ServerConfigurationWrapping, can_override_session_coordinator_methods)
{
    auto const my_session_coordinator = std::dynamic_pointer_cast<MySessionCoordinator>(session_coordinator);

    EXPECT_CALL(*my_session_coordinator, focus_next()).Times(1);
    session_coordinator->focus_next();
}

TEST_F(ServerConfigurationWrapping, returns_same_session_coordinator_from_cache)
{
    ASSERT_THAT(server.the_session_coordinator(), Eq(session_coordinator));
}
