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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir_test_framework/display_server_test_fixture.h"
#include "mir/frontend/session_mediator_report.h"

#include "mir/run_mir.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mf = mir::frontend;
namespace mtf = mir_test_framework;

using namespace testing;

namespace
{
struct MockSessionMediatorReport : mf::NullSessionMediatorReport
{
    MockSessionMediatorReport()
    {
        EXPECT_CALL(*this, session_connect_called(_)).Times(AnyNumber());
        EXPECT_CALL(*this, session_disconnect_called(_)).Times(AnyNumber());

        // These are not needed for the 1st test, but they will be soon
        EXPECT_CALL(*this, session_create_surface_called(_)).Times(AnyNumber());
        EXPECT_CALL(*this, session_release_surface_called(_)).Times(AnyNumber());
        EXPECT_CALL(*this, session_next_buffer_called(_)).Times(AnyNumber());
    }

    MOCK_METHOD1(session_connect_called, void (std::string const&));
    MOCK_METHOD1(session_create_surface_called, void (std::string const&));
    MOCK_METHOD1(session_next_buffer_called, void (std::string const&));
    MOCK_METHOD1(session_release_surface_called, void (std::string const&));
    MOCK_METHOD1(session_disconnect_called, void (std::string const&));
};

struct HostServerConfiguration : public mtf::TestingServerConfiguration
{
    virtual std::shared_ptr<mf::SessionMediatorReport>  the_session_mediator_report()
    {
        if (!mock_session_mediator_report)
            mock_session_mediator_report = std::make_shared<MockSessionMediatorReport>();

        return mock_session_mediator_report;
    }

    std::shared_ptr<MockSessionMediatorReport> mock_session_mediator_report;
};

struct FakeCommandLine
{
    static int const argc = 4;
    char const* argv[argc];

    FakeCommandLine(std::string const& host_socket)
    {
        char const** to = argv;
        for(auto from : { "--file", "NestedServer", "--nested-mode", host_socket.c_str()})
        {
            *to++ = from;
        }

        EXPECT_THAT(to - argv, Eq(argc)); // Check the array size matches parameter list
    }
};

struct NestedServerConfiguration : FakeCommandLine, public mir::DefaultServerConfiguration
{

    NestedServerConfiguration(std::string const& host_socket) :
        FakeCommandLine(host_socket),
        DefaultServerConfiguration(FakeCommandLine::argc, FakeCommandLine::argv)
    {
    }
};

struct ClientConfig : mtf::TestingClientConfiguration
{
    ClientConfig(NestedServerConfiguration& nested_config) : nested_config(nested_config) {}

    NestedServerConfiguration& nested_config;

    void exec() override
    {
        try
        {
            mir::run_mir(nested_config, [](mir::DisplayServer&){});
            // TODO - remove FAIL() as we should exit (NB we need logic to cause exit).
            FAIL();
        }
        catch (std::exception const& x)
        {
            // TODO - this is only temporary until NestedPlatform is implemented.
            EXPECT_THAT(x.what(), HasSubstr("Mir NestedPlatform is not fully implemented yet!"));
        }
    }
};
}

using TestNestedMir = mtf::BespokeDisplayServerTestFixture;

TEST_F(TestNestedMir, nested_platform_connects_and_disconnects)
{
    struct MyHostServerConfiguration : HostServerConfiguration
    {
        void exec() override
        {
            InSequence seq;
            EXPECT_CALL(*mock_session_mediator_report, session_connect_called(_)).Times(1);
            EXPECT_CALL(*mock_session_mediator_report, session_disconnect_called(_)).Times(1);
        }
    };

    MyHostServerConfiguration host_config;
    NestedServerConfiguration nested_config(host_config.the_socket_file());
    ClientConfig client_config(nested_config);


    launch_server_process(host_config);
    launch_client_process(client_config);
}
