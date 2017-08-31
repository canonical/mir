/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
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

#include "mir_toolkit/mir_client_library.h"
#include "mir_toolkit/extensions/graphics_module.h"

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

///\page basic.c basic.c: A simple mir client
/// demo_client shows the use of mir API.
/// This program opens a mir connection and creates a surface.
///\section demo_client demo_client()
/// Opens a mir connection and creates a surface and advances the
/// current buffer before closing the surface and connection.
///\subsection connect request and wait for connection handle
/// \snippet basic.c connect_tag
///\subsection surface_create request and wait for surface handle
/// \snippet basic.c surface_create_tag
///\subsection swap_buffers exchange the current buffer for a new one
/// \snippet basic.c swap_buffers_tag
///\subsection surface_release We release our surface
/// \snippet basic.c surface_release_tag
///\subsection connection_release We release our connection
/// \snippet basic.c connection_release_tag
///\subsection get graphics region for the current buffer
/// \snippet basic.c get_graphics_region_tag
/// \example basic.c A simple mir client
///\section MirDemoState MirDemoState
/// The handles needs to be accessible both to callbacks and to the control function.
/// \snippet basic.c MirDemoState_tag

///\internal [MirDemoState_tag]
// Utility structure for the state of a single surface session.
typedef struct MirDemoState
{
    MirConnection *connection;
    MirWindow* window;
} MirDemoState;
///\internal [MirDemoState_tag]


int demo_client(const char* server, int buffer_swap_count)
{
    static int const width = 640;
    static int const height = 480;

    MirDemoState mcd;
    mcd.connection = 0;
    mcd.window = 0;

    puts("Starting");

    ///\internal [connect_tag]
    mcd.connection = mir_connect_sync(server, __FILE__);
    puts("Connected");
    ///\internal [connect_tag]

    if (mcd.connection == NULL || !mir_connection_is_valid(mcd.connection))
    {
        const char *error = "Unknown error";
        if (mcd.connection != NULL)
            error = mir_connection_get_error_message(mcd.connection);

        fprintf(stderr, "Failed to connect to server `%s': %s\n",
                server == NULL ? "<default>" : server, error);
        return 1;
    }

    {
        MirModuleProperties properties = { NULL, -1, -1, -1, NULL };

        MirExtensionGraphicsModuleV1 const* ext = mir_extension_graphics_module_v1(mcd.connection);
        assert(ext);

        ext->graphics_module(mcd.connection, &properties);

        assert(NULL != properties.name);
        assert(0 <= properties.major_version);
        assert(0 <= properties.minor_version);
        assert(0 <= properties.micro_version);
        assert(NULL != properties.filename);
    }

    // Identify a supported pixel format
    MirPixelFormat pixel_format = mir_pixel_format_invalid;
    unsigned int valid_formats;
    mir_connection_get_available_surface_formats(mcd.connection, &pixel_format, 1, &valid_formats);

    MirRenderSurface* rs = mir_connection_create_render_surface_sync(mcd.connection, width, height);
    MirWindowSpec *spec = mir_create_normal_window_spec(mcd.connection, width, height);
    assert(spec != NULL);
    mir_window_spec_set_name(spec, __FILE__);
    mir_window_spec_add_render_surface(spec, rs, width, height, 0, 0);

    ///\internal [surface_create_tag]
    // ...we create a surface using that format.
    mcd.window = mir_create_window_sync(spec);
    puts("Window created");
    ///\internal [surface_create_tag]

    mir_window_spec_release(spec);

    // We expect a surface handle;
    // we expect it to be valid; and,
    // we don't expect an error description
    assert(mcd.window != NULL);
    if (!mir_window_is_valid(mcd.window))
    {
        fprintf(stderr, "Failed to create surface: %s",
        mir_window_get_error_message(mcd.window));
        return 1;
    }
    else
        assert(strcmp(mir_window_get_error_message(mcd.window), "") == 0);

    MirBufferStream* bs = mir_render_surface_get_buffer_stream(rs, width, height, pixel_format);

    // We can keep exchanging the current buffer for a new one
    for (int i = 0; i < buffer_swap_count; i++)
    {
        // We can query the current graphics buffer attributes
        {
            ///\internal [get_graphics_region_tag]
            MirGraphicsRegion graphics_region;
            mir_buffer_stream_get_graphics_region(bs, &graphics_region);

            ///\internal [get_graphics_region_tag]
            // In a real application we'd render into the graphics_region
        }

        ///\internal [swap_buffers_tag]
        mir_buffer_stream_swap_buffers_sync(bs);
        ///\internal [swap_buffers_tag]
    }

    ///\internal [surface_release_tag]
    // We should release our surface
    mir_window_release_sync(mcd.window);
    puts("Window released");
    ///\internal [surface_release_tag]

    mir_render_surface_release(rs);

    ///\internal [connection_release_tag]
    // We should release our connection
    mir_connection_release(mcd.connection);
    puts("Connection released");
    ///\internal [connection_release_tag]

    return 0;
}

// The main() function deals with parsing arguments and defaults
int main(int argc, char* argv[])
{
    // Some variables for holding command line options
    char const *server = NULL;
    int buffer_swap_count = 0;

    // Parse the command line
    {
        int arg;
        opterr = 0;
        while ((arg = getopt (argc, argv, "c:hm:")) != -1)
        {
            switch (arg)
            {
            case 'c':
                buffer_swap_count = atoi(optarg);
                break;
            case 'm':
                server = optarg;
                break;

            case '?':
            case 'h':
            default:
                puts(argv[0]);
                puts("Usage:");
                puts("    -m <Mir server socket>");
                puts("    -h: this help text");
                return -1;
            }
        }
    }

    return demo_client(server, buffer_swap_count);
}
