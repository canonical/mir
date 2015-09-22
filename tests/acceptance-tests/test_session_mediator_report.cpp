/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "mir_toolkit/mir_client_library.h"
#include "mir_toolkit/mir_prompt_session.h"

#include "mir/frontend/session_mediator_report.h"

#include "mir/test/fake_shared.h"
#include "mir_test_framework/any_surface.h"
#include "mir_test_framework/headless_in_process_server.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mf = mir::frontend;
namespace mt = mir::test;
namespace mtf = mir_test_framework;

using testing::_;

namespace
{

struct MockSessionMediatorReport : mf::SessionMediatorReport
{
    MOCK_METHOD1(session_connect_called, void (std::string const&));
    MOCK_METHOD1(session_create_surface_called, void (std::string const&));
    MOCK_METHOD1(session_next_buffer_called, void (std::string const&));
    MOCK_METHOD1(session_exchange_buffer_called, void (std::string const&));
    MOCK_METHOD1(session_submit_buffer_called, void (std::string const&));
    MOCK_METHOD1(session_allocate_buffers_called, void (std::string const&));
    MOCK_METHOD1(session_release_buffers_called, void (std::string const&));
    MOCK_METHOD1(session_release_surface_called, void (std::string const&));
    MOCK_METHOD1(session_disconnect_called, void (std::string const&));
    MOCK_METHOD2(session_start_prompt_session_called, void (std::string const&, pid_t));
    MOCK_METHOD1(session_stop_prompt_session_called, void (std::string const&));

    void session_configure_surface_called(std::string const&) override {};
    void session_configure_surface_cursor_called(std::string const&) override {};
    void session_configure_display_called(std::string const&) override {};
    void session_error(const std::string&, const char*, const std::string&) override {};
};

void null_prompt_session_state_change_callback(
    MirPromptSession* /*prompt_provider*/, MirPromptSessionState /*state*/,
    void* /*context*/)
{
}

struct SessionMediatorReportTest : mtf::HeadlessInProcessServer
{
    void SetUp() override
    {
        server.override_the_session_mediator_report(
            [this] { return mt::fake_shared(report); });

        mtf::HeadlessInProcessServer::SetUp();
    }

    void TearDown() override
    {
        if (connection)
            mir_connection_release(connection);
        HeadlessInProcessServer::TearDown();
    }

    void connect_client()
    {
        connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);
    }

    void disconnect_client()
    {
        mir_connection_release(connection);
        connection = nullptr;
    }

    testing::NiceMock<MockSessionMediatorReport> report;
    MirConnection* connection = nullptr;
};

}

TEST_F(SessionMediatorReportTest, session_connect_called)
{
    EXPECT_CALL(report, session_connect_called(_));
    connect_client();
}

TEST_F(SessionMediatorReportTest, session_disconnect_called)
{
    connect_client();

    EXPECT_CALL(report, session_disconnect_called(_));
    disconnect_client();
    testing::Mock::VerifyAndClearExpectations(&report);
}

TEST_F(SessionMediatorReportTest, session_create_and_release_surface_called)
{
    connect_client();

    EXPECT_CALL(report, session_create_surface_called(_));
    auto const surface = mtf::make_any_surface(connection);
    testing::Mock::VerifyAndClearExpectations(&report);

    EXPECT_CALL(report, session_release_surface_called(_));
    mir_surface_release_sync(surface);
    testing::Mock::VerifyAndClearExpectations(&report);
}

TEST_F(SessionMediatorReportTest, session_exchange_buffer_called)
{
    EXPECT_CALL(report, session_submit_buffer_called(_));

    connect_client();

    auto const surface = mtf::make_any_surface(connection);
    auto const buffer_stream = mir_surface_get_buffer_stream(surface);
    mir_buffer_stream_swap_buffers_sync(buffer_stream);
    mir_surface_release_sync(surface);
}

TEST_F(SessionMediatorReportTest, session_start_and_stop_prompt_session_called)
{
    connect_client();

    EXPECT_CALL(report, session_start_prompt_session_called(_, _));
    auto const prompt_session = mir_connection_create_prompt_session_sync(
        connection, getpid(), null_prompt_session_state_change_callback, nullptr);
    testing::Mock::VerifyAndClearExpectations(&report);

    EXPECT_CALL(report, session_stop_prompt_session_called(_));
    mir_prompt_session_release_sync(prompt_session);
    testing::Mock::VerifyAndClearExpectations(&report);
}
