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
 * Authored by: Thomas Guest <thomas.guest@canonical.com>
 */

#include "mir_test_framework/display_server_test_fixture.h"

#include "mir_toolkit/mir_client_library.h"
#include "mir_toolkit/mir_client_library_debug.h"
#include "src/client/client_buffer.h"

#include "mir/frontend/connector.h"

#include "mir_protobuf.pb.h"

#ifdef ANDROID
/*
 * MirNativeBuffer for Android is defined opaquely, but we now depend on
 * it having width and height fields, for all platforms. So need definition...
 */
#include <system/window.h>  // for ANativeWindowBuffer AKA MirNativeBuffer
#endif

#include <gtest/gtest.h>
#include <chrono>
#include <thread>
#include <cstring>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <errno.h>

namespace mf = mir::frontend;
namespace mc = mir::compositor;
namespace mcl = mir::client;
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

bool signalled;
static void SIGIO_handler(int /*signo*/)
{
    signalled = true;
}

TEST_F(DefaultDisplayServerTestFixture, ClientLibraryThreadsHandleNoSignals)
{
    struct ClientConfig : ClientConfigCommon
    {
        void exec()
        {
            signalled = false;

            sigset_t sigset;
            sigemptyset(&sigset);
            struct sigaction act;
            act.sa_handler = &SIGIO_handler;
            act.sa_mask = sigset;
            act.sa_flags = 0;
            act.sa_restorer = nullptr;
            if (sigaction(SIGIO, &act, NULL))
                FAIL() << "Failed to set SIGIO action";

            MirConnection* conn = NULL;
            conn = mir_connect_sync(mir_test_socket, __PRETTY_FUNCTION__);

            sigaddset(&sigset, SIGIO);
            pthread_sigmask(SIG_BLOCK, &sigset, NULL);

            // SIGIO should be blocked
            if (kill(getpid(), SIGIO))
                FAIL() << "Failed to send SIGIO signal";

            // Make a roundtrip to the server to ensure the SIGIO has time to be handled
            MirSurfaceParameters const request_params =
            {
                __PRETTY_FUNCTION__,
                640, 480,
                mir_pixel_format_abgr_8888,
                mir_buffer_usage_software,
                mir_display_output_id_invalid
            };

            surface = mir_connection_create_surface_sync(conn, &request_params);

            mir_connection_release(conn);

            EXPECT_FALSE(signalled);
        }
    } client_config;

    launch_client_process(client_config);
}

TEST_F(DefaultDisplayServerTestFixture, ClientLibraryDoesNotInterfereWithClientSignalHandling)
{
    struct ClientConfig : ClientConfigCommon
    {
        void exec()
        {
            signalled = false;

            sigset_t sigset;
            sigemptyset(&sigset);
            struct sigaction act;
            act.sa_handler = &SIGIO_handler;
            act.sa_mask = sigset;
            act.sa_flags = 0;
            act.sa_restorer = nullptr;
            if (sigaction(SIGIO, &act, NULL))
                FAIL() << "Failed to set SIGIO action";

            MirConnection* conn = NULL;
            conn = mir_connect_sync(mir_test_socket, __PRETTY_FUNCTION__);

            // We should receieve SIGIO
            if (kill(getpid(), SIGIO))
                FAIL() << "Failed to send SIGIO signal";

            mir_connection_release(conn);

            EXPECT_TRUE(signalled);
        }
    } client_config;

    launch_client_process(client_config);
}

TEST_F(DefaultDisplayServerTestFixture, MultiSurfaceClientTracksBufferFdsCorrectly)
{
    struct ClientConfig : ClientConfigCommon
    {
        void exec()
        {

            mir_wait_for(mir_connect(mir_test_socket, __PRETTY_FUNCTION__, connection_callback, this));

            ASSERT_TRUE(connection != NULL);
            EXPECT_TRUE(mir_connection_is_valid(connection));
            EXPECT_STREQ("", mir_connection_get_error_message(connection));

            MirSurfaceParameters const request_params =
            {
                __PRETTY_FUNCTION__,
                640, 480,
                mir_pixel_format_abgr_8888,
                mir_buffer_usage_hardware,
                mir_display_output_id_invalid
            };

            surf_one = mir_connection_create_surface_sync(connection, &request_params);
            surf_two = mir_connection_create_surface_sync(connection, &request_params);

            ASSERT_TRUE(surf_one != NULL);
            ASSERT_TRUE(surf_two != NULL);

            buffers = 0;

            while (buffers < 1024)
            {
                mir_surface_swap_buffers_sync(surf_one);
                mir_surface_swap_buffers_sync(surf_two);

                buffers++;
            }

            /* We should not have any stray fds hanging around.
               Test this by trying to open a new one */
            int canary_fd;
            canary_fd = open("/dev/null", O_RDONLY);

            ASSERT_TRUE(canary_fd > 0) << "Failed to open canary file descriptor: "<< strerror(errno);
            EXPECT_TRUE(canary_fd < 1024);

            close(canary_fd);

            mir_wait_for(mir_surface_release(surf_one, release_surface_callback, this));
            mir_wait_for(mir_surface_release(surf_two, release_surface_callback, this));

            ASSERT_TRUE(surf_one == NULL);
            ASSERT_TRUE(surf_two == NULL);

            mir_connection_release(connection);
        }

        virtual void surface_released (MirSurface* surf)
        {
            if (surf == surf_one)
                surf_one = NULL;
            if (surf == surf_two)
                surf_two = NULL;
        }

        MirSurface* surf_one;
        MirSurface* surf_two;
    } client_config;

    launch_client_process(client_config);
}

}
