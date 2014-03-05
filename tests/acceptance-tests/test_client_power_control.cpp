/*
 * Copyright © 2014 Canonical Ltd.
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

static void set_power_modes_to_off(MirDisplayConfiguration *conf)
{
    for (unsigned i = 0; i < conf->num_outputs; i++)
    {
        auto &output = conf->outputs[i];

        output.power_mode = mir_power_mode_off;
    }
}

static void expect_power_modes_are_on(MirDisplayConfiguration *conf)
{
    for (unsigned i = 0; i < conf->num_outputs; i++)
    {
        auto &output = conf->outputs[i];

        EXPECT_EQ(mir_power_mode_on, output.power_mode);
    }
}

static void with_configuration(MirConnection *connection, std::function<void(MirDisplayConfiguration*)> const& exec)
{
    auto conf = mir_connection_create_display_config(connection);
    exec(conf);
    mir_display_config_destroy(conf);
}
}

using ClientPowerControl = BespokeDisplayServerTestFixture;

// When a client has requested to turn the display off, we re-enable it when the client makes a buffer swapping request.
// This behavior is required due to the serial manner in which we process requests on the server side. If the display is off
// when the client makes a swap buffers request, swap buffers will block until the screen is reenabled. However even if on the 
// client side, swap buffers is called asynchronously, the server side RPC thread will not process another request from this client
// until swap buffers completes. Thus its unfortunately not possible for the client to turn the screen back on!
TEST_F(ClientPowerControl, client_advancing_buffer_will_override_power_request_made_by_same_client)
{
    TestingServerConfiguration server_config;
    launch_server_process(server_config);

    struct Client : TestingClientConfiguration
    {
        void exec()
        {
            auto connection = mir_connect_sync(mir_test_socket, __PRETTY_FUNCTION__);

            with_configuration(connection, 
                [&](MirDisplayConfiguration *c)
                {
                    set_power_modes_to_off(c);
                    mir_connection_apply_display_config(connection, c);
                });
            
            MirSurfaceParameters const params =
            {
                __PRETTY_FUNCTION__,
                1, 1,
                mir_pixel_format_abgr_8888,
                mir_buffer_usage_hardware,
                mir_display_output_id_invalid
            };
            auto s = mir_connection_create_surface_sync(connection, &params);

            // Creating a surface advances buffers and thus should flip the power request back to on.
            with_configuration(connection, expect_power_modes_are_on);

            // Flip them back off.
            with_configuration(connection, 
                [&](MirDisplayConfiguration *c)
                {
                    set_power_modes_to_off(c);
                    mir_connection_apply_display_config(connection, c);
                });

            mir_surface_swap_buffers_sync(s);
            with_configuration(connection, expect_power_modes_are_on);
            
            mir_surface_release_sync(s);
            mir_connection_release(connection);
        }
    } client_config;

    launch_client_process(client_config);
}
