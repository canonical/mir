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

#include "mir/shell/input_targeter.h"
#include "mir/shell/shell_wrapper.h"

#include "mir/test/doubles/mock_input_targeter.h"

#include "mir_test_framework/any_surface.h"
#include "mir_test_framework/in_process_server.h"
#include "mir_test_framework/testing_server_configuration.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mf = mir::frontend;
namespace ms = mir::scene;
namespace msh = mir::shell;
namespace mtd = mir::test::doubles;
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

struct ServerConfig : mtf::TestingServerConfiguration
{
    std::shared_ptr<msh::Shell>
    wrap_shell(std::shared_ptr<msh::Shell> const& wrapped) override
    {
        mock_shell = std::make_shared<NiceMock<MockShell>>(wrapped);

        return mock_shell;
    }

    std::shared_ptr<msh::InputTargeter> the_input_targeter() override
    {
        return mock_input_targeter;
    }

    std::shared_ptr<NiceMock<MockShell>> mock_shell;
    std::shared_ptr<NiceMock<mtd::MockInputTargeter>> const mock_input_targeter =
        std::make_shared<NiceMock<mtd::MockInputTargeter>>();
};

struct FocusSelection : mtf::InProcessServer
{
    mir::DefaultServerConfiguration& server_config() override
    {
        return server_configuration;
    }

    MockShell& the_mock_shell()
    {
        return *server_configuration.mock_shell;
    }

    mtd::MockInputTargeter& the_mock_input_targeter()
    {
        return *server_configuration.mock_input_targeter;
    }

    ServerConfig server_configuration;
};
}

TEST_F(FocusSelection, when_client_connects_shell_is_notified_of_session)
{
    InSequence seq;
    EXPECT_CALL(the_mock_shell(), open_session(_, _, _));
    EXPECT_CALL(the_mock_shell(), close_session(NonNullSession()));

    auto const connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);
    ASSERT_TRUE(mir_connection_is_valid(connection));
    mir_connection_release(connection);
}

TEST_F(FocusSelection, when_surface_gets_valid_contents_input_focus_is_set)
{
    EXPECT_CALL(the_mock_input_targeter(), clear_focus()).Times(AtLeast(0));
    EXPECT_CALL(the_mock_input_targeter(), set_focus(_)).Times(1);

    auto const connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);
    ASSERT_TRUE(mir_connection_is_valid(connection));

    auto const surface = mtf::make_any_surface(connection);
    mir_buffer_stream_swap_buffers_sync(mir_surface_get_buffer_stream(surface));
    mir_surface_release_sync(surface);

    mir_connection_release(connection);
}
