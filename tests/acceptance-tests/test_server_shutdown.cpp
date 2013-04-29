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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "mir_toolkit/mir_client_library.h"
#include "mir/graphics/renderer.h"

#include "mir_test_framework/display_server_test_fixture.h"

#include <gtest/gtest.h>

#include <thread>
#include <cstdio>
#include <fcntl.h>

namespace mg = mir::graphics;
namespace mtf = mir_test_framework;

namespace
{

char const* const mir_test_socket = mtf::test_socket_file().c_str();

class NullRenderer : public mg::Renderer
{
public:
    void render(std::function<void(std::shared_ptr<void> const&)>, mg::Renderable&)
    {
        /* 
         * Do nothing, so that the Renderable's buffers are not consumed
         * by the server, thus causing the client to block when asking for
         * the second buffer (assuming double-buffering).
         */
        std::this_thread::yield();
    }
    void ensure_no_live_buffers_bound()
    {
    }
};

void null_surface_callback(MirSurface*, void*)
{
}

}

TEST_F(BespokeDisplayServerTestFixture, server_can_shut_down_when_clients_are_blocked)
{
    static std::string const next_buffer_done1{"next_buffer_done1_c5d49978.tmp"};
    static std::string const next_buffer_done2{"next_buffer_done2_c5d49978.tmp"};
    static std::string const next_buffer_done3{"next_buffer_done3_c5d49978.tmp"};
    static std::string const server_done{"server_done_c5d49978.tmp"};

    std::remove(next_buffer_done1.c_str());
    std::remove(next_buffer_done2.c_str());
    std::remove(next_buffer_done3.c_str());
    std::remove(server_done.c_str());

    struct ServerConfig : TestingServerConfiguration
    {
        std::shared_ptr<mg::Renderer> the_renderer() override
        {
            return renderer([] { return std::make_shared<NullRenderer>(); });
        }
    } server_config;

    launch_server_process(server_config);

    struct ClientConfig : TestingClientConfiguration
    {
        ClientConfig(std::string const& next_buffer_done,
                     std::string const& server_done)
            : next_buffer_done{next_buffer_done},
              server_done{server_done}
        {
        }

        void exec()
        {
            MirConnection* connection = mir_connect_sync(mir_test_socket, __PRETTY_FUNCTION__);

            ASSERT_TRUE(connection != NULL);

            MirSurfaceParameters const request_params =
            {
                __PRETTY_FUNCTION__,
                640, 480,
                mir_pixel_format_abgr_8888,
                mir_buffer_usage_hardware
            };

            MirSurface* surf = mir_connection_create_surface_sync(connection, &request_params);

            /* Ask for the first buffer (should succeed) */
            mir_surface_next_buffer_sync(surf);
            /* Ask for the first second buffer (should block) */
            mir_surface_next_buffer(surf, null_surface_callback, nullptr);

            set_flag(next_buffer_done);
            wait_for(server_done);

            /* 
             * TODO: Releasing the connection to a shut down server blocks
             * the client. We should handle unexpected server shutdown more
             * gracefully on the client side.
             *
             * mir_connection_release(connection);
             */
        }

        void set_flag(std::string const& flag_file)
        {
            close(open(flag_file.c_str(), O_CREAT, S_IWUSR | S_IRUSR));
        }

        void wait_for(std::string const& flag_file)
        {
            int fd = -1;
            while ((fd = open(flag_file.c_str(), O_RDONLY, S_IWUSR | S_IRUSR)) == -1)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
            close(fd);
        }

        std::string const next_buffer_done;
        std::string const server_done;
    };
    
    ClientConfig client_config1{next_buffer_done1, server_done};
    ClientConfig client_config2{next_buffer_done2, server_done};
    ClientConfig client_config3{next_buffer_done3, server_done};

    launch_client_process(client_config1);
    launch_client_process(client_config2);
    launch_client_process(client_config3);

    run_in_test_process([&]
    {
        /* Wait until the clients are blocked on getting the second buffer */
        client_config1.wait_for(next_buffer_done1);
        client_config2.wait_for(next_buffer_done2);
        client_config3.wait_for(next_buffer_done3);

        /* Shutting down the server should not block */
        shutdown_server_process();

        /* Notify the clients that we are done (we only need to set the flag once) */
        client_config1.set_flag(server_done);
    });
}
