/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir/graphics/renderer.h"
#include "mir_client/mir_client_library.h"
#include "mir/thread/all.h"

#include "mir_test_framework/display_server_test_fixture.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mg = mir::graphics;
namespace mtf = mir_test_framework;

namespace
{
struct Client : TestingClientConfiguration
{
    void exec()
    {
        mir_wait_for(mir_connect(
            mtf::test_socket_file().c_str(),
            __PRETTY_FUNCTION__,
            connection_callback,
            this));

        MirSurfaceParameters const request_params =
        {
            __PRETTY_FUNCTION__,
            640, 480,
            mir_pixel_format_rgba_8888,
            mir_buffer_usage_hardware
        };

        mir_wait_for(mir_surface_create(connection, &request_params, create_surface_callback, this));
        mir_wait_for(mir_surface_next_buffer(surface, next_buffer_callback, this));
        mir_wait_for(mir_surface_next_buffer(surface, next_buffer_callback, this));
        mir_wait_for(mir_surface_release( surface, release_surface_callback, this));

        mir_connection_release(connection);
    }

    Client() : connection(0), surface(0), buffers(0)
    {
    }

    static void connection_callback(MirConnection* connection, void* context)
    {
        Client* self = reinterpret_cast<Client*>(context);
        self->connection = connection;
    }

    static void create_surface_callback(MirSurface* surface, void* context)
    {
        Client* self = reinterpret_cast<Client*>(context);
        self->surface_created(surface);
    }

    static void next_buffer_callback(MirSurface* surface, void* context)
    {
        Client* self = reinterpret_cast<Client*>(context);
        self->next_buffer(surface);
    }

    static void release_surface_callback(MirSurface* surface, void* context)
    {
        Client* self = reinterpret_cast<Client*>(context);
        self->surface_released(surface);
    }

    virtual void connected(MirConnection* new_connection)
    {
        connection = new_connection;
    }

    virtual void surface_created(MirSurface* new_surface)
    {
        surface = new_surface;
    }

    virtual void next_buffer(MirSurface*)
    {
        ++buffers;
    }

    virtual void surface_released(MirSurface* /*released_surface*/)
    {
        surface = NULL;
    }

    MirConnection* connection;
    MirSurface* surface;
    int buffers;
};

struct Server : TestingServerConfiguration
{
    std::shared_ptr<mg::Renderer> make_renderer(
        std::shared_ptr<mg::Display> const& /*display*/)
    {
        class StubRenderer : public mg::Renderer
        {
        public:
            virtual void render(mg::Renderable& /*r*/)
            {
                // NOOP will cause client threads to stall
            }
        };
        return std::make_shared<StubRenderer>();
    }
};
}

namespace mir_test_framework
{
struct FrontendShutdown : BespokeDisplayServerTestFixture
{
    Server server_processing;
    Client client_config;

    void wait_for_client_to_connect()
    {
        // How do we detect that client has started a session?
        // Also, only need this delay in testing process.
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
};
}
using mir_test_framework::FrontendShutdown;


TEST_F(FrontendShutdown, after_client_connects)
{
    launch_server_process(server_processing);

    launch_client_process(client_config);

    wait_for_client_to_connect();

    // The problem we're really testing for is a hang here
    // ...so there ought to be some sort of timeout.
    // Maybe that should be in Process::wait_for_termination()?
    EXPECT_TRUE(shutdown_server_process());

    // TODO: clients detect that server failed - and can exit themselves
    kill_client_processes();
}

// TODO Fix behaviour and enable test
// Disabled because this leaves a server with a single thread spinning
// waiting for "signal_display_server" to be set. I don't see how the
// main thread gets cancelled without the signal handler completing.
// Maybe this is just an artifact of the frig in that code?
TEST_F(FrontendShutdown, DISABLED_before_client_connects)
{
    launch_server_process(server_processing);

    // The problem we're really testing for is a hang here
    // ...so there ought to be some sort of timeout.
    // Maybe that should be in Process::wait_for_termination()?
    EXPECT_TRUE(shutdown_server_process());
}
