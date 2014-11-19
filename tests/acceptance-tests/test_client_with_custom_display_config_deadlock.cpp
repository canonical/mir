/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "mir_toolkit/mir_client_library.h"

#include "mir_test_framework/connected_client_headless_server.h"

#include <gtest/gtest.h>

using ClientWithCustomDisplayConfiguration = mir_test_framework::ConnectedClientHeadlessServer;

// Regression test for LP:#1340669
// Test is not deterministic since we are testing a race, but failure can be
// reproduced easily with repeated runs.
TEST_F(ClientWithCustomDisplayConfiguration,
       does_not_deadlock_server_with_existing_client_when_disconnecting)
{
    auto second_connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);
    MirSurfaceParameters const request_params =
    {
        __PRETTY_FUNCTION__,
        640, 480,
        mir_pixel_format_abgr_8888,
        mir_buffer_usage_hardware,
        mir_display_output_id_invalid
    };

    auto second_surface = mir_connection_create_surface_sync(second_connection, &request_params);
    ASSERT_TRUE(mir_surface_is_valid(second_surface));

    auto configuration = mir_connection_create_display_config(connection);
    mir_wait_for(mir_connection_apply_display_config(connection, configuration));
    EXPECT_STREQ("", mir_connection_get_error_message(connection));
    mir_display_config_destroy(configuration);

    mir_connection_release(second_connection);

    // Server (and therefore the test) will deadlock and won't be able to
    // shut down without the fix. It's not ideal to deadlock on test failure,
    // but it's the best check we have at the moment.
}
