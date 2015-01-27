/*
 * Copyright © 2014-2015 Canonical Ltd.
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
#include "mir/shell/shell_wrapper.h"

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
struct MyShell : msh::ShellWrapper
{
    using msh::ShellWrapper::ShellWrapper;
    MOCK_METHOD1(handle_surface_created, void(std::shared_ptr<ms::Session> const&));
};

struct MySessionCoordinator : msh::SessionCoordinatorWrapper
{
    using msh::SessionCoordinatorWrapper::SessionCoordinatorWrapper;

    MOCK_METHOD0(unset_focus, void());
};

struct ServerConfigurationWrapping : mir_test_framework::HeadlessTest
{
    void SetUp() override
    {
        server.wrap_shell([]
            (std::shared_ptr<msh::Shell> const& wrapped)
            {
                return std::make_shared<MyShell>(wrapped);
            });

        server.wrap_session_coordinator([]
            (std::shared_ptr<ms::SessionCoordinator> const& wrapped)
            {
                return std::make_shared<MySessionCoordinator>(wrapped);
            });

        server.apply_settings();

        shell = server.the_shell();
        session_coordinator = server.the_session_coordinator();
    }

    std::shared_ptr<msh::Shell> shell;
    std::shared_ptr<ms::SessionCoordinator> session_coordinator;
};
}

TEST_F(ServerConfigurationWrapping, shell_is_of_wrapper_type)
{
    auto const my_shell = std::dynamic_pointer_cast<MyShell>(shell);

    EXPECT_THAT(my_shell, Ne(nullptr));
}

TEST_F(ServerConfigurationWrapping, can_override_shell_methods)
{
    auto const my_shell = std::dynamic_pointer_cast<MyShell>(shell);

    EXPECT_CALL(*my_shell, handle_surface_created(_)).Times(1);
    shell->handle_surface_created({});
}

TEST_F(ServerConfigurationWrapping, returns_same_shell_from_cache)
{
    ASSERT_THAT(server.the_shell(), Eq(shell));
}

TEST_F(ServerConfigurationWrapping, session_coordinator_is_of_wrapper_type)
{
    auto const my_session_coordinator = std::dynamic_pointer_cast<MySessionCoordinator>(session_coordinator);

    EXPECT_THAT(my_session_coordinator, Ne(nullptr));
}

TEST_F(ServerConfigurationWrapping, can_override_session_coordinator_methods)
{
    auto const my_session_coordinator = std::dynamic_pointer_cast<MySessionCoordinator>(session_coordinator);

    EXPECT_CALL(*my_session_coordinator, unset_focus()).Times(1);
    session_coordinator->unset_focus();
}

TEST_F(ServerConfigurationWrapping, returns_same_session_coordinator_from_cache)
{
    ASSERT_THAT(server.the_session_coordinator(), Eq(session_coordinator));
}
