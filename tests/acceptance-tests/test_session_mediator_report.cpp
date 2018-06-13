/*
 * Copyright © 2015-2018 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
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

#include "mir/frontend/session_mediator_observer.h"
#include "mir/observer_registrar.h"

#include "mir/test/fake_shared.h"
#include "mir_test_framework/any_surface.h"
#include "mir_test_framework/headless_in_process_server.h"
#include "mir/test/signal.h"


#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mf = mir::frontend;
namespace mt = mir::test;
namespace mtf = mir_test_framework;

using namespace testing;
using namespace std::chrono_literals;

namespace
{

struct MockSessionMediatorReport : mf::SessionMediatorObserver
{
    MOCK_METHOD1(session_connect_called, void (std::string const&));
    MOCK_METHOD1(session_create_surface_called, void (std::string const&));
    MOCK_METHOD1(session_submit_buffer_called, void (std::string const&));
    MOCK_METHOD1(session_allocate_buffers_called, void (std::string const&));
    MOCK_METHOD1(session_release_buffers_called, void (std::string const&));
    MOCK_METHOD1(session_release_surface_called, void (std::string const&));
    MOCK_METHOD1(session_disconnect_called, void (std::string const&));
    MOCK_METHOD2(session_start_prompt_session_called, void (std::string const&, pid_t));
    MOCK_METHOD1(session_stop_prompt_session_called, void (std::string const&));

    MOCK_METHOD1(session_create_buffer_stream_called, void (std::string const& app_name));
    MOCK_METHOD1(session_release_buffer_stream_called, void (std::string const& app_name));

    void session_configure_surface_called(std::string const&) override {};
    void session_configure_surface_cursor_called(std::string const&) override {};
    void session_configure_display_called(std::string const&) override {};
    void session_set_base_display_configuration_called(std::string const&) override {};
    void session_preview_base_display_configuration_called(std::string const&) override {};
    void session_confirm_base_display_configuration_called(std::string const&) override {};
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
        mtf::HeadlessInProcessServer::SetUp();

        server.the_session_mediator_observer_registrar()->register_interest(
            report);
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

    std::shared_ptr<testing::NiceMock<MockSessionMediatorReport>> report{
        std::make_shared<testing::NiceMock<MockSessionMediatorReport>>()};
    MirConnection* connection = nullptr;
};

}

TEST_F(SessionMediatorReportTest, session_connect_called)
{
    auto report_received = std::make_shared<mt::Signal>();
    EXPECT_CALL(*report, session_connect_called(_))
        .WillOnce(InvokeWithoutArgs([report_received]() { report_received->raise(); }));
    connect_client();
    EXPECT_TRUE(report_received->wait_for(10s));
}

TEST_F(SessionMediatorReportTest, session_disconnect_called)
{
    connect_client();

    auto report_received = std::make_shared<mt::Signal>();
    ON_CALL(*report, session_disconnect_called(_))
        .WillByDefault(InvokeWithoutArgs([report_received]() { report_received->raise(); }));
    disconnect_client();
    EXPECT_TRUE(report_received->wait_for(10s));
}

TEST_F(SessionMediatorReportTest, session_create_and_release_surface_called)
{
    connect_client();

    auto report_received = std::make_shared<mt::Signal>();
    ON_CALL(*report, session_create_surface_called(_))
        .WillByDefault(InvokeWithoutArgs([report_received]() { report_received->raise(); }));
    auto const window = mtf::make_any_surface(connection);
    EXPECT_TRUE(report_received->wait_for(10s));

    report_received->reset();

    ON_CALL(*report, session_release_surface_called(_))
        .WillByDefault(InvokeWithoutArgs([report_received]() { report_received->raise(); }));
    mir_window_release_sync(window);
    EXPECT_TRUE(report_received->wait_for(10s));
}

TEST_F(SessionMediatorReportTest, session_submit_buffer_called)
{
    using namespace testing;
    using namespace std::chrono_literals;

    auto report_received = std::make_shared<mt::Signal>();
    ON_CALL(*report, session_submit_buffer_called(_))
        .WillByDefault(InvokeWithoutArgs([report_received]() { report_received->raise(); }));


    connect_client();

    auto const window = mtf::make_any_surface(connection);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    auto const buffer_stream = mir_window_get_buffer_stream(window);
#pragma GCC diagnostic pop
    mir_buffer_stream_swap_buffers_sync(buffer_stream);
    mir_window_release_sync(window);
    EXPECT_TRUE(report_received->wait_for(10s));
}

TEST_F(SessionMediatorReportTest, session_start_and_stop_prompt_session_called)
{
    connect_client();

    auto report_received = std::make_shared<mt::Signal>();
    ON_CALL(*report, session_start_prompt_session_called(_, _))
        .WillByDefault(InvokeWithoutArgs([report_received]() { report_received->raise(); }));

    auto const prompt_session = mir_connection_create_prompt_session_sync(
        connection, getpid(), null_prompt_session_state_change_callback, nullptr);
    EXPECT_TRUE(report_received->wait_for(10s));

    report_received->reset();

    ON_CALL(*report, session_stop_prompt_session_called(_))
        .WillByDefault(InvokeWithoutArgs([report_received]() { report_received->raise(); }));
    mir_prompt_session_release_sync(prompt_session);
    EXPECT_TRUE(report_received->wait_for(10s));
}

TEST_F(SessionMediatorReportTest, session_create_and_release_buffer_stream_called)
{
    connect_client();

    auto report_received = std::make_shared<mt::Signal>();
    ON_CALL(*report, session_create_buffer_stream_called(_))
        .WillByDefault(InvokeWithoutArgs([report_received]() { report_received->raise(); }));
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    auto const buffer_stream = mir_connection_create_buffer_stream_sync(connection,
        640, 480, mir_pixel_format_abgr_8888, mir_buffer_usage_software);
#pragma GCC diagnostic pop
    EXPECT_TRUE(report_received->wait_for(10s));

    report_received->reset();

    ON_CALL(*report, session_release_buffer_stream_called(_))
        .WillByDefault(InvokeWithoutArgs([report_received]() { report_received->raise(); }));

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    mir_buffer_stream_release_sync(buffer_stream);
#pragma GCC diagnostic pop
    EXPECT_TRUE(report_received->wait_for(10s));
}
