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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir_client/mir_client_library_lightdm.h"

#include "mir/chrono/chrono.h"
#include "mir/thread/all.h"

#include "mir_test_framework/display_server_test_fixture.h"

#include <gtest/gtest.h>

namespace mtf = mir_test_framework;

namespace
{
    char const* const mir_test_socket = mtf::test_socket_file().c_str();
}

// TODO frig to make it compile
MirWaitHandle *mir_connect_with_lightdm_id(
    char const *server,
    int /*lightdm_id*/,
    char const *app_name,
    mir_connected_callback callback,
    void *client_context)
{
    return mir_connect(
        server,
        app_name,
        callback,
        client_context);
}

// TODO frig to make it compile
void mir_select_focus_by_lightdm_id(int /*lightdm_id*/)
{
}


namespace mir
{
namespace
{
struct ClientConfigCommon : TestingClientConfiguration
{
    ClientConfigCommon() :
        connection(0),
        surface(0),
        buffers(0)
    {
    }

    static void connection_callback(MirConnection* connection, void* context)
    {
        ClientConfigCommon* config = reinterpret_cast<ClientConfigCommon *>(context);
        config->connection = connection;
    }

    static void create_surface_callback(MirSurface* surface, void* context)
    {
        ClientConfigCommon* config = reinterpret_cast<ClientConfigCommon *>(context);
        config->surface_created(surface);
    }

    static void next_buffer_callback(MirSurface* surface, void* context)
    {
        ClientConfigCommon* config = reinterpret_cast<ClientConfigCommon *>(context);
        config->next_buffer(surface);
    }

    static void release_surface_callback(MirSurface* surface, void* context)
    {
        ClientConfigCommon* config = reinterpret_cast<ClientConfigCommon *>(context);
        config->surface_released(surface);
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
        buffers.fetch_add(1);
    }

    virtual void surface_released(MirSurface* /*released_surface*/)
    {
        surface = NULL;
    }

    MirConnection* connection;
    MirSurface* surface;
    std::atomic<int> buffers;
};
}

TEST_F(DefaultDisplayServerTestFixture, focus_management)
{
    int const lightdm_id = __LINE__;
    struct ClientConfig : ClientConfigCommon
    {
        void exec()
        {
            mir_wait_for(mir_connect_with_lightdm_id(
                mir_test_socket,
                lightdm_id,
                __PRETTY_FUNCTION__,
                connection_callback,
                this));

            ASSERT_TRUE(connection != NULL);

            // TODO Create a suface and wait for test to finish

            mir_connection_release(connection);
        }
    } client_config;

    launch_client_process(client_config);

    // TODO Wait for client to init
    mir_select_focus_by_lightdm_id(int lightdm_id);

    // TODO check that focus was set.
}
}
