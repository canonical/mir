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

#include "mir/geometry/rectangle.h"
#include "mir/graphics/display_buffer.h"
#include "mir/compositor/renderer.h"
#include "mir/compositor/renderer_factory.h"
#include "mir/compositor/compositor.h"
#include "mir/compositor/display_buffer_compositor.h"
#include "mir/compositor/display_buffer_compositor_factory.h"
#include "mir/compositor/scene.h"

#include "mir_test_framework/display_server_test_fixture.h"
#include "mir_test_doubles/null_display.h"
#include "mir_test_doubles/stub_display_buffer.h"

#include "mir_toolkit/mir_client_library.h"

#include <gtest/gtest.h>

#include <unordered_map>
#include <thread>
#include <unistd.h>
#include <fcntl.h>

namespace geom = mir::geometry;
namespace mg = mir::graphics;
namespace mc = mir::compositor;
namespace ms = mir::surfaces;
namespace mtf = mir_test_framework;
namespace mtd = mir::test::doubles;

namespace
{

char const* const mir_test_socket = mtf::test_socket_file().c_str();

class SynchronousCompositor : public mc::Compositor
{
public:
    SynchronousCompositor(std::shared_ptr<mg::Display> const& display,
                          std::shared_ptr<mc::Scene> const& scene,
                          std::shared_ptr<mc::DisplayBufferCompositorFactory> const& db_compositor_factory)
        : display{display},
          scene{scene}
    {
        display->for_each_display_buffer(
            [this, &db_compositor_factory](mg::DisplayBuffer& display_buffer)
            {
                display_buffer_compositor_map[&display_buffer] = db_compositor_factory->create_compositor_for(display_buffer);
            });
}

    void start()
    {
        scene->set_change_callback([this]()
        {
            display->for_each_display_buffer([this](mg::DisplayBuffer& display_buffer)
            {
                display_buffer_compositor_map[&display_buffer]->composite();
            });
        });
    }

    void stop()
    {
        scene->set_change_callback([]{});
    }

private:
    std::shared_ptr<mg::Display> const display;
    std::shared_ptr<mc::Scene> const scene;
    std::unordered_map<mg::DisplayBuffer*,std::unique_ptr<mc::DisplayBufferCompositor>> display_buffer_compositor_map;
};

class StubRenderer : public mc::Renderer
{
public:
    StubRenderer(int render_operations_fd)
        : render_operations_fd{render_operations_fd}
    {
    }

    void clear(unsigned long) override {}

    void render(std::function<void(std::shared_ptr<void> const&)>, mc::CompositingCriteria const&, mc::BufferStream&)
    {
        while (write(render_operations_fd, "a", 1) != 1) continue;
    }

private:
    int render_operations_fd;
};

class StubDisplay : public mtd::NullDisplay
{
public:
    StubDisplay()
        : display_buffer{geom::Rectangle{geom::Point{}, geom::Size{1600,1600}}}
    {
    }

    void for_each_display_buffer(std::function<void(mg::DisplayBuffer&)> const& f) override
    {
        f(display_buffer);
    }

private:
    mtd::StubDisplayBuffer display_buffer;
};

}

using SurfaceFirstFrameSync = BespokeDisplayServerTestFixture;

TEST_F(SurfaceFirstFrameSync, surface_not_rendered_until_buffer_is_pushed)
{
    static std::string const surface_created{"surface_created_0950f3f9.tmp"};
    static std::string const do_next_buffer{"do_next_buffer_0950f3f9.tmp"};
    static std::string const next_buffer_done{"next_buffer_done_0950f3f9.tmp"};
    static std::string const do_client_finish{"do_client_finish_0950f3f9.tmp"};

    std::remove(surface_created.c_str());
    std::remove(do_next_buffer.c_str());
    std::remove(next_buffer_done.c_str());
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

        ~ServerConfig() noexcept
        {
            if (rendering_ops_pipe[0] >= 0)
                close(rendering_ops_pipe[0]);
            if (rendering_ops_pipe[1] >= 0)
                close(rendering_ops_pipe[1]);
        }

        std::shared_ptr<mg::Display> the_display() override
        {
            using namespace testing;

            if (!stub_display)
                stub_display = std::make_shared<StubDisplay>();

            return stub_display;
        }

        std::shared_ptr<mc::RendererFactory> the_renderer_factory() override
        {
            using namespace testing;
            struct StubRendererFactory : public mc::RendererFactory
            {
                StubRendererFactory(int ops_fd) : ops_fd{ops_fd} {}
                std::unique_ptr<mc::Renderer> create_renderer_for(geom::Rectangle const&)
                {
                    auto raw = new StubRenderer{ops_fd};
                    return std::unique_ptr<StubRenderer>(raw);
                }

                int ops_fd;
            };

            if (!stub_renderer_factory)
                stub_renderer_factory = std::make_shared<StubRendererFactory>(rendering_ops_pipe[1]);

            return stub_renderer_factory;
        }

        std::shared_ptr<mc::Compositor> the_compositor() override
        {
            using namespace testing;

            if (!sync_compositor)
            {
                sync_compositor =
                    std::make_shared<SynchronousCompositor>(
                        the_display(),
                        the_scene(),
                        the_display_buffer_compositor_factory());
            }

            return sync_compositor;
        }

        int num_of_executed_render_operations()
        {
            char c;
            int ops{0};

            while (read(rendering_ops_pipe[0], &c, 1) == 1)
                ops++;

            return ops;
        }

        int rendering_ops_pipe[2];
        std::shared_ptr<mc::RendererFactory> stub_renderer_factory;
        std::shared_ptr<StubDisplay> stub_display;
        std::shared_ptr<SynchronousCompositor> sync_compositor;
    } server_config;

    launch_server_process(server_config);

    struct ClientConfig : TestingClientConfiguration
    {
        ClientConfig(std::string const& surface_created,
                     std::string const& do_next_buffer,
                     std::string const& next_buffer_done,
                     std::string const& do_client_finish)
            : surface_created{surface_created},
              do_next_buffer{do_next_buffer},
              next_buffer_done{next_buffer_done},
              do_client_finish{do_client_finish}
        {
        }

        void exec()
        {
            auto connection = mir_connect_sync(mir_test_socket, __PRETTY_FUNCTION__);

            ASSERT_TRUE(connection != NULL);
            EXPECT_TRUE(mir_connection_is_valid(connection));
            EXPECT_STREQ(mir_connection_get_error_message(connection), "");

            MirSurfaceParameters const request_params =
            {
                __PRETTY_FUNCTION__,
                640, 480,
                mir_pixel_format_abgr_8888,
                mir_buffer_usage_hardware,
                mir_display_output_id_invalid
            };

            auto surface = mir_connection_create_surface_sync(connection, &request_params);

            ASSERT_TRUE(surface != NULL);
            EXPECT_TRUE(mir_surface_is_valid(surface));
            EXPECT_STREQ(mir_surface_get_error_message(surface), "");

            set_flag(surface_created);
            wait_for(do_next_buffer);

            mir_surface_swap_buffers_sync(surface);

            set_flag(next_buffer_done);
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

        std::string const surface_created;
        std::string const do_next_buffer;
        std::string const next_buffer_done;
        std::string const do_client_finish;
    } client_config{surface_created, do_next_buffer,
                    next_buffer_done, do_client_finish};

    launch_client_process(client_config);

    run_in_test_process([&]
    {
        client_config.wait_for(surface_created);

        /*
         * This test uses a synchronous compositor (instead of the default
         * multi-threaded one) to ensure we don't get a false negative
         * for this expectation. In the multi-threaded case this can
         * happen if the compositor doesn't get the chance to run at all
         * before we reach this point.
         */
        EXPECT_EQ(0, server_config.num_of_executed_render_operations());

        client_config.set_flag(do_next_buffer);
        client_config.wait_for(next_buffer_done);

        /* After submitting the buffer we should get some render operations */
        while (server_config.num_of_executed_render_operations() == 0)
            std::this_thread::sleep_for(std::chrono::milliseconds(1));

        client_config.set_flag(do_client_finish);
    });
}
