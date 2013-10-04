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

#include "mir_toolkit/mir_client_library.h"

#include "mir_test_framework/display_server_test_fixture.h"
#include "mir_test_framework/cross_process_sync.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <atomic>

namespace mtf = mir_test_framework;

using ServerDisconnect = mtf::BespokeDisplayServerTestFixture;

namespace
{
struct MockEventHandler
{
    MOCK_METHOD1(handle, void(MirLifecycleState transition));

    static void handle(MirConnection*, MirLifecycleState transition, void* event_handler)
    {
        static_cast<MockEventHandler*>(event_handler)->handle(transition);
    }
};

struct MyTestingClientConfiguration : mtf::TestingClientConfiguration
{
    void exec() override
    {
        static MirSurfaceParameters const params =
            { __PRETTY_FUNCTION__, 33, 45, mir_pixel_format_abgr_8888,
              mir_buffer_usage_hardware, mir_display_output_id_invalid };

        MockEventHandler mock_event_handler;

        auto connection = mir_connect_sync(mtf::test_socket_file().c_str() , __PRETTY_FUNCTION__);
        mir_connection_set_lifecycle_event_callback(connection, &MockEventHandler::handle, &mock_event_handler);
        auto surface = mir_connection_create_surface_sync(connection, &params);

        std::atomic<bool> signalled(false);

        EXPECT_CALL(mock_event_handler, handle(mir_lifecycle_connection_lost)).Times(1).
            WillOnce(testing::InvokeWithoutArgs([&] { signalled.store(true); }));

        sync.signal_ready();

        using clock = std::chrono::high_resolution_clock;

        auto time_limit = clock::now() + std::chrono::seconds(2);

        while (!signalled.load() && clock::now() < time_limit)
        {
            mir_surface_swap_buffers_sync(surface);
        }
    }

    mtf::CrossProcessSync sync;
};
}

TEST_F(ServerDisconnect, client_detects_server_shutdown)
{
    TestingServerConfiguration server_config;
    launch_server_process(server_config);

    MyTestingClientConfiguration client_config;
    launch_client_process(client_config);

    run_in_test_process([this, &client_config]
    {
        client_config.sync.wait_for_signal_ready_for();
        shutdown_server_process();
    });
}
