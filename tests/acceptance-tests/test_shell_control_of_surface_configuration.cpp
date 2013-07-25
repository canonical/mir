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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#include "mir_test_framework/display_server_test_fixture.h"

#include "mir_toolkit/mir_client_library.h"


#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mtf = mir_test_framework;

namespace
{
    char const* const mir_test_socket = mtf::test_socket_file().c_str();
}

namespace
{

struct SurfaceCreatingClient : public mtf::TestingClientConfiguration
{
    void exec() override
    {
        connection = mir_connect_sync(mir_test_socket, __PRETTY_FUNCTION__);
        ASSERT_TRUE(connection != NULL);
        EXPECT_TRUE(mir_connection_is_valid(connection));
        EXPECT_STREQ(mir_connection_get_error_message(connection), "");

        MirSurfaceParameters const request_params =
            {
                __PRETTY_FUNCTION__,
                640, 480,
                mir_pixel_format_abgr_8888,
                mir_buffer_usage_hardware
            };

        surface = mir_connection_create_surface_sync(connection,
                                                     &request_params);
    }

    MirConnection* connection;
    MirSurface* surface;
};

}

TEST_F(BespokeDisplayServerTestFixture, the_shell_surface_configurator_is_notified_of_attribute_changes)
{
    struct ServerConfiguration : TestingServerConfiguration
    {
    } server_config;
    launch_server_process(server_config);

    struct ClientConfiguration : SurfaceCreatingClient
    {
        void exec() override
        {
            SurfaceCreatingClient::exec();
        }
    } client_config;
    launch_client_process(client_config);
}
