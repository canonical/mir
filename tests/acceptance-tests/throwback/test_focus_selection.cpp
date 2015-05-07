/*
 * Copyright © 2013-2015 Canonical Ltd.
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

#include "mir/shell/shell_wrapper.h"
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

struct MockShell : msh::ShellWrapper
{
    explicit MockShell(std::shared_ptr<msh::Shell> const& wrapped) :
        msh::ShellWrapper{wrapped}
    {
        ON_CALL(*this, open_session(_, _, _)).
            WillByDefault(Invoke(this, &MockShell::unmocked_open_session));

        ON_CALL(*this, close_session(_)).
            WillByDefault(Invoke(this, &MockShell::unmocked_close_session));
    }

    MOCK_METHOD3(open_session, std::shared_ptr<ms::Session>(
        pid_t, std::string const&, std::shared_ptr<mf::EventSink> const&));

    MOCK_METHOD1(close_session, void (std::shared_ptr<ms::Session> const&));

private:

    std::shared_ptr<ms::Session> unmocked_open_session(
        pid_t client_pid,
        std::string const& name,
        std::shared_ptr<mf::EventSink> const& sink)
    { return msh::ShellWrapper::open_session(client_pid, name, sink); }

    void unmocked_close_session(std::shared_ptr<ms::Session> const& session)
    { msh::ShellWrapper::close_session(session); }
};
}

using FocusSelection = BespokeDisplayServerTestFixture;

TEST_F(FocusSelection, when_surface_created_shell_is_notified_of_session)
{
    struct ServerConfig : TestingServerConfiguration
    {
        std::shared_ptr<msh::Shell>
        wrap_shell(std::shared_ptr<msh::Shell> const& wrapped) override
        {
            sm = std::make_shared<MockShell>(wrapped);

            InSequence seq;
            EXPECT_CALL(*sm, open_session(_, _, _));
            EXPECT_CALL(*sm, close_session(NonNullSession()));

            return sm;
        }

        std::shared_ptr<MockShell> sm;
    } server_config;

    launch_server_process(server_config);

    mtf::SurfaceCreatingClient client;

    launch_client_process(client);
}

TEST_F(FocusSelection, when_surface_created_input_focus_is_set)
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

                EXPECT_CALL(*targeter, clear_focus()).Times(AtLeast(0));

                {
                    InSequence seq;
                    EXPECT_CALL(*targeter, set_focus(_)).Times(1);
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
