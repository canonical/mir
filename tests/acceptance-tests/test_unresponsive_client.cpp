/*
 * Copyright © 2014 Canonical Ltd.
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

#include "src/server/scene/session_container.h"
#include "mir/scene/session.h"
#include "mir/scene/surface.h"

#include "mir_test/cross_process_action.h"
#include "mir_test_framework/display_server_test_fixture.h"

#include "mir_toolkit/mir_client_library.h"

#include <gtest/gtest.h>

#include <string>
#include <thread>

namespace mt = mir::test;
namespace mtf = mir_test_framework;

namespace
{
char const* const mir_test_socket = mtf::test_socket_file().c_str();
}
using UnresponsiveClient = mtf::BespokeDisplayServerTestFixture;

TEST_F(UnresponsiveClient, does_not_hang_server)
{
    struct ServerConfig : TestingServerConfiguration
    {
        void on_start() override
        {
            std::thread(
                [this]
                {
                    send_events.exec(
                    [this]
                    {
                        auto const sessions = the_session_container();

                        for (int i = 0; i < 1000; ++i)
                        {
                            sessions->for_each(
                                [i] (std::shared_ptr<mir::scene::Session> const& session)
                                {
                                    session->default_surface()->resize({i + 1, i + 1});
                                });
                        }
                    });
                }).detach();
        }
        mt::CrossProcessAction send_events;
    } server_config;

    launch_server_process(server_config);

    struct ClientConfig : TestingClientConfiguration
    {
        void exec()
        {
            MirConnection* connection = nullptr;
            MirSurface* surface = nullptr;

            connect.exec(
                [&]
                {
                    connection = mir_connect_sync(mir_test_socket, __PRETTY_FUNCTION__);

                    MirSurfaceParameters const request_params =
                    {
                        __PRETTY_FUNCTION__,
                        100, 100,
                        mir_pixel_format_abgr_8888,
                        mir_buffer_usage_hardware,
                        mir_display_output_id_invalid
                    };
                    surface = mir_connection_create_surface_sync(connection, &request_params);
                });

            release.exec(
                [&]
                {
                    // We would normally explicitly release the surface at this
                    // point. However, because we have been filling the server
                    // send socket buffer, releasing the surface may cause the
                    // server to try to write to a full socket buffer when
                    // responding, leading the server to believe that the client
                    // is blocked, and causing a premature client disconnection.
                    mir_connection_release(connection);
                });
        }

        mt::CrossProcessAction connect;
        mt::CrossProcessAction release;
    } client_config;

    auto const client_pid = launch_client_process(client_config);

    run_in_test_process([&]
    {
        client_config.connect();
        kill(client_pid, SIGSTOP);
        server_config.send_events(std::chrono::seconds{10});
        kill(client_pid, SIGCONT);
        client_config.release();
    });
}
