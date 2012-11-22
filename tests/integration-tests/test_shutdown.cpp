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
struct ClientConfigCommon : TestingClientConfiguration
{
    ClientConfigCommon()
        : connection(0)
        , surface(0),
        buffers(0)
    {
    }

    static void connection_callback(MirConnection * connection, void * context)
    {
        ClientConfigCommon * config = reinterpret_cast<ClientConfigCommon *>(context);
        config->connection = connection;
    }

    static void create_surface_callback(MirSurface * surface, void * context)
    {
        ClientConfigCommon * config = reinterpret_cast<ClientConfigCommon *>(context);
        config->surface_created(surface);
    }

    static void next_buffer_callback(MirSurface * surface, void * context)
    {
        ClientConfigCommon * config = reinterpret_cast<ClientConfigCommon *>(context);
        config->next_buffer(surface);
    }

    static void release_surface_callback(MirSurface * surface, void * context)
    {
        ClientConfigCommon * config = reinterpret_cast<ClientConfigCommon *>(context);
        config->surface_released(surface);
    }

    virtual void connected(MirConnection * new_connection)
    {
        connection = new_connection;
    }

    virtual void surface_created(MirSurface * new_surface)
    {
        surface = new_surface;
    }

    virtual void next_buffer(MirSurface*)
    {
        ++buffers;
    }

    virtual void surface_released(MirSurface * /*released_surface*/)
    {
        surface = NULL;
    }

    MirConnection* connection;
    MirSurface* surface;
    int buffers;
};
}

TEST_F(BespokeDisplayServerTestFixture, frontend_shutdown_race)
{
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

    } server_processing;

    launch_server_process(server_processing);

    struct ClientConfig : ClientConfigCommon
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
    } client_config;

    launch_client_process(client_config);

    // How do we detect that client has started a session?
    std::this_thread::sleep_for(std::chrono::seconds(1));

    EXPECT_TRUE(shutdown_server_process());

    // TODO: clients detect that server failed - and can exit themselves
    kill_client_processes();
}




