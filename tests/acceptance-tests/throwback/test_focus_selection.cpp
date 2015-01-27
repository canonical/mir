/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#include "clients.h"

#include "mir/shell/session_coordinator_wrapper.h"
#include "mir/graphics/display.h"
#include "mir/shell/input_targeter.h"

#include "mir_test_framework/display_server_test_fixture.h"
#include "mir_test_doubles/mock_input_targeter.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mf = mir::frontend;
namespace ms = mir::scene;
namespace msh = mir::shell;
namespace mi = mir::input;
namespace mt = mir::test;
namespace mtd = mt::doubles;
namespace mt = mir::test;
namespace mtf = mir_test_framework;

using namespace testing;

namespace
{
MATCHER(NonNullSession, "")
{
    return arg.operator bool();
}

struct MockSessionManager : msh::SessionCoordinatorWrapper
{
    explicit MockSessionManager(std::shared_ptr<ms::SessionCoordinator> const& wrapped) :
        msh::SessionCoordinatorWrapper{wrapped}
    {
        ON_CALL(*this, set_focus_to(_)).
            WillByDefault(Invoke(this, &MockSessionManager::unmocked_set_focus_to));

        ON_CALL(*this, unset_focus()).
            WillByDefault(Invoke(this, &MockSessionManager::unmocked_unset_focus));
}

    MOCK_METHOD1(set_focus_to, void(std::shared_ptr<ms::Session> const& focus));
    MOCK_METHOD0(unset_focus, void());

private:
    void unmocked_set_focus_to(std::shared_ptr<ms::Session> const& focus)
    { msh::SessionCoordinatorWrapper::set_focus_to(focus); }

    void unmocked_unset_focus()
    { msh::SessionCoordinatorWrapper::unset_focus(); }
};
}

TEST_F(BespokeDisplayServerTestFixture, sessions_creating_surface_receive_focus)
{
    struct ServerConfig : TestingServerConfiguration
    {
        std::shared_ptr<ms::SessionCoordinator>
        wrap_session_coordinator(
            std::shared_ptr<ms::SessionCoordinator> const& wrapped) override
        {
            sm = std::make_shared<MockSessionManager>(wrapped);

            InSequence seq;
            // Once on application registration and once on surface creation
            EXPECT_CALL(*sm, set_focus_to(NonNullSession())).Times(2);
            // Focus is cleared when the session is closed
            EXPECT_CALL(*sm, unset_focus()).Times(1);

            return sm;
        }

        std::shared_ptr<MockSessionManager> sm;
    } server_config;

    launch_server_process(server_config);

    mtf::SurfaceCreatingClient client;

    launch_client_process(client);
}

TEST_F(BespokeDisplayServerTestFixture, surfaces_receive_input_focus_when_created)
{
    struct ServerConfig : TestingServerConfiguration
    {
        std::shared_ptr<mtd::MockInputTargeter> targeter;
        bool expected;

        ServerConfig()
          : targeter(std::make_shared<mtd::MockInputTargeter>()),
            expected(false)
        {
        }

        std::shared_ptr<msh::InputTargeter>
        the_input_targeter() override
        {
            using namespace ::testing;

            if (!expected)
            {

                EXPECT_CALL(*targeter, focus_cleared()).Times(AtLeast(0));

                {
                    InSequence seq;
                    EXPECT_CALL(*targeter, focus_changed(_)).Times(1);
                    expected = true;
                }
            }

            return targeter;
        }
    } server_config;


    launch_server_process(server_config);

    mtf::SurfaceCreatingClient client;

    launch_client_process(client);
}
