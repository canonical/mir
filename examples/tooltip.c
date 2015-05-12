/*
 *
 * Copyright © 2015 Canonical Ltd.
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
 * Author: Alan Griffiths <alan@octopull.co.uk>
 */

#include "eglapp.h"
#include <mir_toolkit/mir_client_library.h>

#include <stdio.h>
#include <unistd.h>
#include <GLES2/gl2.h>

///\example tooltip.c
/// A simple orange client surface with a simple grey tooltip

static MirPixelFormat select_pixel_format(MirConnection* connection);
static MirSurface* create_tooltip(MirConnection* const connection, MirSurface* const parent, const MirPixelFormat format);

typedef struct Color
{
    GLfloat r, g, b, a;
} Color;

int main(int argc, char *argv[])
{
    float const opacity = mir_eglapp_background_opacity;
    Color const orange = {0.866666667f, 0.282352941f, 0.141414141f, opacity};

    unsigned int width = 300, height = 200;

    if (!mir_eglapp_init(argc, argv, &width, &height))
        return 1;

    glClearColor(orange.r, orange.g, orange.b, orange.a);
    glClear(GL_COLOR_BUFFER_BIT);
    mir_eglapp_swap_buffers();

    MirConnection* const connection = mir_eglapp_native_connection();
    MirSurface* const parent = mir_eglapp_native_surface();

    MirSurfaceSpec* const spec = mir_connection_create_spec_for_changes(connection);
    mir_surface_spec_set_name(spec, "tooltip example");
    mir_surface_spec_set_min_width(spec, width/2);
    mir_surface_spec_set_max_width(spec, width*2);
    mir_surface_spec_set_min_height(spec, height/2);
    mir_surface_spec_set_max_height(spec, height*2);
    mir_surface_apply_spec(parent, spec);
    mir_surface_spec_release(spec);

    MirSurface* tooltip = create_tooltip(connection, parent, select_pixel_format(connection));
    while (mir_eglapp_running())
    {
    }

    mir_surface_release_sync(tooltip);
    mir_eglapp_shutdown();

    return 0;
}

static MirPixelFormat select_pixel_format(MirConnection* connection)
{
    unsigned int format[mir_pixel_formats];
    unsigned int nformats;

    mir_connection_get_available_surface_formats(connection,
        format, mir_pixel_formats, &nformats);

    MirPixelFormat pixel_format = format[0];
    for (unsigned int f = 0; f < nformats; f++)
    {
        const bool opaque = (format[f] == mir_pixel_format_xbgr_8888 ||
                            format[f] == mir_pixel_format_xrgb_8888 ||
                            format[f] == mir_pixel_format_bgr_888);

        if (opaque)
        {
            pixel_format = format[f];
            break;
        }
    }

    return pixel_format;
}

static MirSurface* create_tooltip(MirConnection* const connection, MirSurface* const parent, const MirPixelFormat format)
{
    MirRectangle zone = { 0, 0, 10, 10 };
    int const width = 50;
    int const height = 20;
    MirSurfaceSpec* const spec = mir_connection_create_spec_for_tooltip(
        connection, width, height, format, parent, &zone);

    mir_surface_spec_set_buffer_usage(spec, mir_buffer_usage_software);
    mir_surface_spec_set_name(spec, "tooltip");
    mir_surface_spec_set_min_width(spec, width);
    mir_surface_spec_set_max_width(spec, width);
    mir_surface_spec_set_min_height(spec, height);
    mir_surface_spec_set_max_height(spec, height);

    MirSurface* tooltip = mir_surface_create_sync(spec);
    mir_surface_spec_release(spec);

    MirBufferStream* const bs = mir_surface_get_buffer_stream(tooltip);
    MirGraphicsRegion buffer;
    mir_buffer_stream_get_graphics_region(bs, &buffer);

    for (int y = 0; y != buffer.height; ++y)
    {
        for (int n = 0; n != buffer.width; ++n)
            switch (format)
            {
            case mir_pixel_format_abgr_8888:
            case mir_pixel_format_argb_8888:
            {
                uint32_t* const pixel = (uint32_t*) (buffer.vaddr + y * buffer.stride);
                pixel[n] = 0xff7f7f7f;
                break;
            }
            case mir_pixel_format_xbgr_8888:
            case mir_pixel_format_xrgb_8888:
            {
                uint32_t* const pixel = (uint32_t*) (buffer.vaddr + y * buffer.stride);
                pixel[n] = 0x007f7f7f;
                break;
            }
            case mir_pixel_format_bgr_888:
            {
                uint8_t* const subpixel = (uint8_t*) (buffer.vaddr + y * buffer.stride);
                subpixel[3 * n + 0] = 0x7f;
                subpixel[3 * n + 1] = 0x7f;
                subpixel[3 * n + 2] = 0x7f;
                break;
            }
            default:
                break;
            }
    }

    mir_buffer_stream_swap_buffers_sync(bs);

    return tooltip;
}
