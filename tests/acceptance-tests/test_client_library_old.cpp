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

}
