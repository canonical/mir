/*
 * Copyright © 2013-2015 Canonical Ltd.
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

#include "mir/compositor/display_buffer_compositor_factory.h"
#include "mir/compositor/display_buffer_compositor.h"
#include "mir/run_mir.h"
#include "mir/main_loop.h"

#include "mir_toolkit/mir_client_library.h"

#include "mir_test_framework/fake_input_server_configuration.h"
#include "mir_test_framework/fake_input_device.h"
#include "mir_test_framework/stub_server_platform_factory.h"
#include "mir_test_framework/any_surface.h"
#include "mir_test_framework/server_runner.h"
#include "mir_test_framework/testing_server_configuration.h"

#include "mir/test/doubles/null_display_buffer_compositor_factory.h"

#include "mir/test/doubles/stub_renderer.h"

#include "mir/dispatch/action_queue.h"
#include "mir/input/input_device.h"
#include "mir/input/input_device_info.h"
#include "mir/input/input_device_registry.h"

#include "mir_toolkit/mir_client_library.h"

#include <gtest/gtest.h>

namespace mtf = mir_test_framework;
namespace mtd = mir::test::doubles;
namespace mc = mir::compositor;
namespace mg = mir::graphics;
namespace mi = mir::input;

namespace
{

struct ServerShutdown : testing::Test, mtf::ServerRunner
{
    std::unique_ptr<mir::DefaultServerConfiguration> server_configuration;

    mir::DefaultServerConfiguration& server_config() override
    {
        return *server_configuration;
    }
};

void null_buffer_stream_callback(MirBufferStream*, void*)
{
}

void null_lifecycle_callback(MirConnection*, MirLifecycleState, void*)
{
}

struct ClientWithSurface
{
    ClientWithSurface(std::string const& server_socket)
        : connection{mir_connect_sync(
            server_socket.c_str(), __PRETTY_FUNCTION__)}
    {

        // Default lifecycle handler terminates the process on disconnect, so
        // override it
        mir_connection_set_lifecycle_event_callback(
            connection, null_lifecycle_callback, nullptr);
        window = mtf::make_any_surface(connection);
    }

    ~ClientWithSurface()
    {
        mir_window_release_sync(window);
        mir_connection_release(connection);
    }

    void swap_sync()
    {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
        mir_buffer_stream_swap_buffers_sync(
            mir_window_get_buffer_stream(window));
#pragma GCC diagnostic pop
    }

    void swap_async()
    {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
        mir_buffer_stream_swap_buffers(
            mir_window_get_buffer_stream(window),
            null_buffer_stream_callback, nullptr);
#pragma GCC diagnostic pop
    }

    MirConnection* const connection;
    MirWindow* window;
};

}

TEST_F(ServerShutdown, server_can_shut_down_when_clients_are_blocked)
{
    struct ServerConfig : mtf::TestingServerConfiguration
    {
        // Don't consume frames, so the clients can block waiting for a buffer
        std::shared_ptr<mc::DisplayBufferCompositorFactory> the_display_buffer_compositor_factory() override
        {
            return display_buffer_compositor_factory(
                [] { return std::make_shared<mtd::NullDisplayBufferCompositorFactory>(); });
        }
    };
    server_configuration.reset(new ServerConfig{});

    start_server();

    ClientWithSurface client1{new_connection()};
    ClientWithSurface client2{new_connection()};
    ClientWithSurface client3{new_connection()};

    client1.swap_sync();
    // Ask for the second buffer (should block)
    client1.swap_async();

    client2.swap_sync();
    // Ask for the second buffer (should block)
    client2.swap_async();

    client3.swap_sync();
    // Ask for the second buffer (should block)
    client3.swap_async();

    // Shutting down the server should not block
    stop_server();

    // Kill the ServerConfiguration to ensure the Clients are unblocked.
    server_configuration.reset();
}

TEST_F(ServerShutdown, server_releases_resources_on_shutdown_with_connected_clients)
{
    server_configuration.reset(new mtf::TestingServerConfiguration{});

    start_server();

    ClientWithSurface client1{new_connection()};
    ClientWithSurface client2{new_connection()};
    ClientWithSurface client3{new_connection()};

    stop_server();

    std::weak_ptr<mir::graphics::Display> display = server_configuration->the_display();
    std::weak_ptr<mir::compositor::Compositor> compositor = server_configuration->the_compositor();
    std::weak_ptr<mir::frontend::Connector> connector = server_configuration->the_connector();
    std::weak_ptr<mi::InputManager> input_manager = server_configuration->the_input_manager();

    server_configuration.reset();

    EXPECT_EQ(0, display.use_count());
    EXPECT_EQ(0, compositor.use_count());
    EXPECT_EQ(0, connector.use_count());
    EXPECT_EQ(0, input_manager.use_count());
}

namespace
{

class ExceptionThrowingDisplayBufferCompositorFactory : public mc::DisplayBufferCompositorFactory
{
public:
    std::unique_ptr<mc::DisplayBufferCompositor>
        create_compositor_for(mg::DisplayBuffer&) override
    {
        struct ExceptionThrowingDisplayBufferCompositor : mc::DisplayBufferCompositor
        {
            void composite(mc::SceneElementSequence&&) override
            {
                throw std::runtime_error("ExceptionThrowingDisplayBufferCompositor");
            }
        };

        return std::unique_ptr<mc::DisplayBufferCompositor>(
            new ExceptionThrowingDisplayBufferCompositor{});
    }
};

}

TEST(ServerShutdownWithThreadException,
     server_releases_resources_on_abnormal_compositor_thread_termination)
{
    struct ServerConfig : mtf::TestingServerConfiguration
    {
        std::shared_ptr<mc::DisplayBufferCompositorFactory>
            the_display_buffer_compositor_factory() override
        {
            return display_buffer_compositor_factory(
                []()
                {
                    return std::make_shared<ExceptionThrowingDisplayBufferCompositorFactory>();
                });
        }
    };

    auto server_config = std::make_shared<ServerConfig>();

    std::thread server{
        [&]
        {
            EXPECT_THROW(
                mir::run_mir(*server_config, [](mir::DisplayServer&){}),
                std::runtime_error);
        }};

    server.join();

    std::weak_ptr<mir::graphics::Display> display = server_config->the_display();
    std::weak_ptr<mir::compositor::Compositor> compositor = server_config->the_compositor();
    std::weak_ptr<mir::frontend::Connector> connector = server_config->the_connector();
    std::weak_ptr<mi::InputManager> input_manager = server_config->the_input_manager();
    std::weak_ptr<mi::InputDeviceHub> hub = server_config->the_input_device_hub();

    server_config.reset();

    EXPECT_EQ(0, display.use_count());
    EXPECT_EQ(0, compositor.use_count());
    EXPECT_EQ(0, connector.use_count());
    EXPECT_EQ(0, input_manager.use_count());
    EXPECT_EQ(0, hub.use_count());
}

TEST(ServerShutdownWithThreadException,
     server_releases_resources_on_abnormal_input_thread_termination)
{
    auto server_config = std::make_shared<mtf::FakeInputServerConfiguration>();

    std::thread server{
        [&server_config]
        {
            EXPECT_THROW(
                mir::run_mir(*server_config, [](mir::DisplayServer&){}),
                std::runtime_error);
        }};

    {
        auto dev = mtf::add_fake_input_device(mir::input::InputDeviceInfo{"throwing device", "throwing-device-0",
                                                                          mir::input::DeviceCapability::unknown});
        dev->emit_runtime_error();
        server.join();
        dev.reset();
    }

    std::weak_ptr<mir::graphics::Display> display = server_config->the_display();
    std::weak_ptr<mir::compositor::Compositor> compositor = server_config->the_compositor();
    std::weak_ptr<mir::frontend::Connector> connector = server_config->the_connector();
    std::weak_ptr<mi::InputManager> input_manager = server_config->the_input_manager();
    std::weak_ptr<mi::InputDeviceHub> hub = server_config->the_input_device_hub();

    server_config.reset();

    EXPECT_EQ(0, display.use_count());
    EXPECT_EQ(0, compositor.use_count());
    EXPECT_EQ(0, connector.use_count());
    EXPECT_EQ(0, input_manager.use_count());
    EXPECT_EQ(0, hub.use_count());
    }

// This also acts as a regression test for LP: #1378740
TEST(ServerShutdownWithThreadException,
     server_releases_resources_on_abnormal_main_thread_termination)
{
    // Use the FakeInputServerConfiguration to get the production input components
    auto server_config = std::make_shared<mtf::FakeInputServerConfiguration>();

    std::thread server{
        [&]
        {
            EXPECT_THROW(
                mir::run_mir(*server_config,
                    [server_config](mir::DisplayServer&)
                    {
                        server_config->the_main_loop()->enqueue(
                            server_config.get(),
                            [] { throw std::runtime_error(""); });
                    }),
                std::runtime_error);
        }};

    server.join();

    std::weak_ptr<mir::graphics::Display> display = server_config->the_display();
    std::weak_ptr<mir::compositor::Compositor> compositor = server_config->the_compositor();
    std::weak_ptr<mir::frontend::Connector> connector = server_config->the_connector();
    std::weak_ptr<mi::InputManager> input_manager = server_config->the_input_manager();
    std::weak_ptr<mi::InputDeviceHub> hub = server_config->the_input_device_hub();

    server_config.reset();

    EXPECT_EQ(0, display.use_count());
    EXPECT_EQ(0, compositor.use_count());
    EXPECT_EQ(0, connector.use_count());
    EXPECT_EQ(0, input_manager.use_count());
    EXPECT_EQ(0, hub.use_count());
}
