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

#include "mir_client/mir_client_library.h"

#include <unistd.h>

static MirConnection *connection = 0;
static MirSurface *surface = 0;

// TODO dirty, dirty hack because stdatomic.h is missing
static volatile int connection_callback_context = 0;
static volatile int surface_create_context = 0;
static volatile int surface_release_context = 0;

static void connection_callback(MirConnection *new_connection, void *context)
{
    connection = new_connection;
    *((int*)context) = 1;
}

static void wait_for_connect()
{
    while (!connection_callback_context) sleep(0);
}

static void surface_create_callback(MirSurface *new_surface, void *context)
{
    surface = new_surface;
    *((int*)context) = 1;
}

static void wait_for_surface_create()
{
    while (!surface_create_context) sleep(0);
}

static void surface_release_callback(MirSurface *old_surface, void *context)
{
    surface = 0;
    *((int*)context) = 1;
}

static void wait_for_surface_release()
{
    while (!surface_release_context) sleep(0);
}

int main()
{
    puts("Starting");

    mir_connect(
        "/tmp/mir_socket", __PRETTY_FUNCTION__,
        connection_callback, (void*)&connection_callback_context);
    wait_for_connect();
    puts("Connected");

//    ASSERT_TRUE(connection != NULL);
//    EXPECT_TRUE(mir_connection_is_valid(connection));
//    EXPECT_STREQ(mir_connection_get_error_message(connection), "");

    MirSurfaceParameters const request_params =
        {__PRETTY_FUNCTION__, 640, 480, mir_pixel_format_rgba_8888};
    mir_surface_create(connection, &request_params, surface_create_callback, (void*)&surface_create_context);
    wait_for_surface_create();
    puts("Surface created");

//    ASSERT_TRUE(ssync->surface != NULL);
//    EXPECT_TRUE(mir_surface_is_valid(ssync->surface));
//    EXPECT_STREQ(mir_surface_get_error_message(ssync->surface), "");

    MirSurfaceParameters response_params;
    mir_surface_get_parameters(surface, &response_params);
//    EXPECT_EQ(request_params.width, response_params.width);
//    EXPECT_EQ(request_params.height, response_params.height);
//    EXPECT_EQ(request_params.pixel_format, response_params.pixel_format);


    mir_surface_release(surface, surface_release_callback, (void*)&surface_release_context);
    wait_for_surface_release();
    puts("Surface released");

//    ASSERT_TRUE(ssync->surface == NULL);

    mir_connection_release(connection);
    puts("Connection released");

    return 0;
}



