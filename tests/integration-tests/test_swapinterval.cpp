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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir/geometry/rectangle.h"
#include "mir/graphics/display.h"
#include "mir/graphics/display_buffer.h"
#include "mir/compositor/renderer.h"
#include "mir/compositor/compositor.h"
#include "mir/compositor/display_buffer_compositor.h"
#include "mir/compositor/scene.h"
#include "mir/surfaces/buffer_stream.h"
#include "mir/surfaces/buffer_stream_factory.h"

#include "mir_test_framework/display_server_test_fixture.h"
#include "mir_test_doubles/stub_buffer.h"

#include "mir_toolkit/mir_client_library.h"

#include <gtest/gtest.h>

#include <thread>
#include <unistd.h>
#include <fcntl.h>

namespace geom = mir::geometry;
namespace mg = mir::graphics;
namespace mc = mir::compositor;
namespace mtf = mir_test_framework;
namespace ms = mir::surfaces;
namespace mtd = mir::test::doubles;

namespace
{
char const* const mir_test_socket = mtf::test_socket_file().c_str();

class CountingBufferStream : public ms::BufferStream
{
public:
    CountingBufferStream(int render_operations_fd)
     : render_operations_fd(render_operations_fd)
    {
    }

    std::shared_ptr<mg::Buffer> secure_client_buffer() { return std::make_shared<mtd::StubBuffer>(); }
    std::shared_ptr<mg::Buffer> lock_compositor_buffer() { return std::make_shared<mtd::StubBuffer>(); }
    std::shared_ptr<mg::Buffer> lock_snapshot_buffer() { return std::make_shared<mtd::StubBuffer>(); }
    geom::PixelFormat get_stream_pixel_format() { return geom::PixelFormat::abgr_8888; }
    geom::Size stream_size() { return geom::Size{}; }
    void force_requests_to_complete() {}
    void allow_framedropping(bool)
    {
        while (write(render_operations_fd, "a", 1) != 1) continue;
    }

private:
    int render_operations_fd; 
};

class StubStreamFactory : public ms::BufferStreamFactory
{
public:
    StubStreamFactory(int render_operations_fd)
     : render_operations_fd(render_operations_fd)
    {
    }

    std::shared_ptr<ms::BufferStream> create_buffer_stream(mg::BufferProperties const&)
    {
        return std::make_shared<CountingBufferStream>(render_operations_fd);
    }
private:
    int render_operations_fd; 
};

}

using SwapIntervalSignalTest = BespokeDisplayServerTestFixture;
TEST_F(SwapIntervalSignalTest, swapinterval_test)
{
    static std::string const swapinterval_set{"swapinterval_set_952f3f10.tmp"};
    static std::string const do_client_finish{"do_client_finish_952f3f10.tmp"};

    std::remove(swapinterval_set.c_str());
    std::remove(do_client_finish.c_str());

    struct ServerConfig : TestingServerConfiguration
    {
        ServerConfig()
        {
            if (pipe(rendering_ops_pipe) != 0)
            {
                BOOST_THROW_EXCEPTION(
                    std::runtime_error("Failed to create pipe"));
            }

            if (fcntl(rendering_ops_pipe[0], F_SETFL, O_NONBLOCK) != 0)
            {
                BOOST_THROW_EXCEPTION(
                    std::runtime_error("Failed to make the read end of the pipe non-blocking"));
            }
        }

        ~ServerConfig()
        {
            if (rendering_ops_pipe[0] >= 0)
                close(rendering_ops_pipe[0]);
            if (rendering_ops_pipe[1] >= 0)
                close(rendering_ops_pipe[1]);
        }

        std::shared_ptr<ms::BufferStreamFactory> the_buffer_stream_factory() override
        {
            if (!stub_stream_factory)
                stub_stream_factory = std::make_shared<StubStreamFactory>(rendering_ops_pipe[1]);
            return stub_stream_factory;
        }

        int num_of_swapinterval_commands()
        {
            char c;
            int ops{0};

            while (read(rendering_ops_pipe[0], &c, 1) == 1)
                ops++;

            return ops;
        }

        int rendering_ops_pipe[2];
        std::shared_ptr<StubStreamFactory> stub_stream_factory;
    } server_config;

    launch_server_process(server_config);

    struct ClientConfig : TestingClientConfiguration
    {
        ClientConfig(std::string const& swapinterval_set,
                     std::string const& do_client_finish)
            : swapinterval_set{swapinterval_set},
              do_client_finish{do_client_finish}
        {
        }

        static void surface_callback(MirSurface*, void*)
        {
        }

        void exec()
        {
            MirSurfaceParameters request_params =
            {
                __PRETTY_FUNCTION__,
                640, 480,
                mir_pixel_format_abgr_8888,
                mir_buffer_usage_hardware,
                mir_display_output_id_invalid
            };

            MirConnection* connection = mir_connect_sync(mir_test_socket, "testapp");
            MirSurface* surface = mir_connection_create_surface_sync(connection, &request_params);

            //1 is the default swapinterval
            EXPECT_EQ(1, mir_surface_get_swapinterval(surface));

            mir_wait_for(mir_surface_set_swapinterval(surface, 0));
            EXPECT_EQ(0, mir_surface_get_swapinterval(surface));

            mir_wait_for(mir_surface_set_swapinterval(surface, 1));
            EXPECT_EQ(1, mir_surface_get_swapinterval(surface));

            //swapinterval 2 not supported
            EXPECT_EQ(NULL, mir_surface_set_swapinterval(surface, 2));
            EXPECT_EQ(1, mir_surface_get_swapinterval(surface)); 

            set_flag(swapinterval_set);
            wait_for(do_client_finish);

            mir_surface_release_sync(surface);
            mir_connection_release(connection);
        }

        /* TODO: Extract this flag mechanism and make it reusable */
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

        std::string const swapinterval_set;
        std::string const do_client_finish;
    } client_config{swapinterval_set, do_client_finish};

    launch_client_process(client_config);

    run_in_test_process([&]
    {
        client_config.wait_for(swapinterval_set);

        EXPECT_EQ(2, server_config.num_of_swapinterval_commands());

        client_config.set_flag(do_client_finish);
    });
}
