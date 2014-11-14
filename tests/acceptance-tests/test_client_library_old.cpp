/*
 * Copyright © 2012-2014 Canonical Ltd.
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

#include "mir_test_framework/interprocess_client_server_test.h"

#include "mir_toolkit/mir_client_library.h"

#include <gtest/gtest.h>

#include <sys/types.h>

namespace mtf = mir_test_framework;

namespace
{
bool signalled;
}

static void SIGIO_handler(int /*signo*/)
{
    signalled = true;
}

struct ClientLibraryThread : mtf::InterprocessClientServerTest
{
    void SetUp() override
    {
        mtf::InterprocessClientServerTest::SetUp();
        run_in_server([]{});
    }
};

TEST_F(ClientLibraryThread, handles_no_signals)
{
    run_in_client([&]
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

            MirConnection* conn = mir_connect_sync(mir_test_socket, __PRETTY_FUNCTION__);

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

            MirSurface* surface = mir_connection_create_surface_sync(conn, &request_params);
            mir_surface_release_sync(surface);
            mir_connection_release(conn);

            EXPECT_FALSE(signalled);
        });
}

TEST_F(ClientLibraryThread, does_not_interfere_with_client_signal_handling)
{
    run_in_client([&]
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

            MirConnection* conn = mir_connect_sync(mir_test_socket, __PRETTY_FUNCTION__);

            // We should receieve SIGIO
            if (kill(getpid(), SIGIO))
                FAIL() << "Failed to send SIGIO signal";

            mir_connection_release(conn);

            EXPECT_TRUE(signalled);
        });
}
