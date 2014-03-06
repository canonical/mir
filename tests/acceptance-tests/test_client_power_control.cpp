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

static void set_power_modes_to_on(MirDisplayConfiguration *conf)
{
    for (unsigned i = 0; i < conf->num_outputs; i++)
    {
        auto &output = conf->outputs[i];

        output.power_mode = mir_power_mode_on;
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

static void expect_power_modes_are_off(MirDisplayConfiguration *conf)
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

// This is a regression test for an old situation. Before next_buffer was executed
// asynchronously with respect to the session mediator it was possible for a client
// to reach an impossible to unblock state:
// 1. Turn power off.
// 2. Swaps buffers.
// Now the client RPC thread was blocked and swap buffers will never complete so we
// can never turn the display back on.
// This test verifies that this scenario no longer results in a deadlock.
TEST_F(ClientPowerControl, client_may_turn_power_back_on_while_blocked_on_swap_buffers)
{
    TestingServerConfiguration server_config;
    launch_server_process(server_config);

    struct Client : TestingClientConfiguration
    {
        MirSurface *surface;
        static void surface_callback(MirSurface *s, void* ctx)
        {
            Client *c = static_cast<Client*>(ctx);
            c->surface = s;
        }
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
            // Creating the surface will try to swap buffers, so this can not complete
            // while the display is off.
            auto s_handle = mir_connection_create_surface(connection, &params, surface_callback, this);

            with_configuration(connection, expect_power_modes_are_off);

            // Flip them back on.
            with_configuration(connection, 
                [&](MirDisplayConfiguration *c)
                {
                    set_power_modes_to_on(c);
                    mir_connection_apply_display_config(connection, c);
                });

            with_configuration(connection, expect_power_modes_are_on);
            
            mir_wait_for(s_handle);
            
            EXPECT_NE(nullptr, surface);
                        
            mir_surface_release_sync(surface);
            mir_connection_release(connection);
        }
    } client_config;

    launch_client_process(client_config);
}
