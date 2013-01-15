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

#include <fcntl.h>

namespace mtf = mir_test_framework;

namespace
{
    char const* const mir_test_socket = mtf::test_socket_file().c_str();
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

    void set_flag(const char* const flag_file)
    {
        close(open(flag_file, O_CREAT, S_IWUSR | S_IRUSR));
    }

    void wait_flag(const char* const flag_file)
    {
        int fd = -1;
        while (-1 == (fd = open(flag_file, O_RDONLY, S_IWUSR | S_IRUSR)))
        {
            // Wait for focus_config process to init
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        close(fd);
    }
};


}

TEST_F(BespokeDisplayServerTestFixture, focus_management)
{
    struct ServerConfig : TestingServerConfiguration
    {
        std::shared_ptr<sessions::SessionStore>
        make_session_store(std::shared_ptr<sessions::SurfaceFactory> const& surface_factory)
        {
            auto real_session_store =
             DefaultServerConfiguration::make_session_store(surface_factory);

            // TODO wrap the real_session_store and check focus gets set

            return real_session_store;
        }
    } server_config;

    launch_server_process(server_config);

    int const lightdm_id = __LINE__;
    static char const* const focus_ready = "focus_management_focus_client_ready.tmp";
    static char const* const manager_done = "focus_management_focus_manager_done.tmp";
    std::remove(focus_ready);
    std::remove(manager_done);

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

            MirSurfaceParameters const request_params =
            {
                __PRETTY_FUNCTION__,
                640, 480,
                mir_pixel_format_abgr_8888,
                mir_buffer_usage_hardware
            };

            mir_wait_for(mir_surface_create(connection, &request_params, create_surface_callback, this));

            set_flag(focus_ready);

            wait_flag(manager_done);
            mir_connection_release(connection);
        }
    } focus_config;

    struct LightdmConfig : ClientConfigCommon
    {
        void exec()
        {
            mir_wait_for(mir_connect(
                mir_test_socket,
                __PRETTY_FUNCTION__,
                connection_callback,
                this));

            wait_flag(focus_ready);
            mir_select_focus_by_lightdm_id(connection, lightdm_id);

            // TODO check focus gets set

            mir_connection_release(connection);
            set_flag(manager_done);
        }
    } lightdm_config;

    launch_client_process(focus_config);
    launch_client_process(lightdm_config);
}
}
