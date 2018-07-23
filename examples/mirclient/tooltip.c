/*
 *
 * Copyright © 2015 Canonical Ltd.
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

typedef struct MyWindow
{
    MirRenderSurface    *surface;
    MirWindow           *window;
} MyWindow;

static MyWindow create_tooltip(MirConnection* const connection, MirWindow* const parent, const MirPixelFormat format);

typedef struct Color
{
    GLfloat r, g, b, a;
} Color;

int main(int argc, char *argv[])
{
    float const opacity = mir_eglapp_background_opacity;
    Color const orange = {0.866666667f, 0.282352941f, 0.141414141f, opacity};

    unsigned int width = 300, height = 200;

    if (!mir_eglapp_init(argc, argv, &width, &height, NULL))
        return 1;

    glClearColor(orange.r, orange.g, orange.b, orange.a);
    glClear(GL_COLOR_BUFFER_BIT);
    mir_eglapp_swap_buffers();

    MirConnection* const connection = mir_eglapp_native_connection();
    MirWindow* const parent = mir_eglapp_native_window();

    MirWindowSpec* const spec = mir_create_window_spec(connection);
    mir_window_spec_set_name(spec, "tooltip example");
    mir_window_spec_set_min_width(spec, width/2);
    mir_window_spec_set_max_width(spec, width*2);
    mir_window_spec_set_min_height(spec, height/2);
    mir_window_spec_set_max_height(spec, height*2);
    mir_window_apply_spec(parent, spec);
    mir_window_spec_release(spec);

    MyWindow tooltip = create_tooltip(connection, parent, select_pixel_format(connection));
    while (mir_eglapp_running())
    {
    }

    mir_window_release_sync(tooltip.window);
    mir_render_surface_release(tooltip.surface);
    mir_eglapp_cleanup();

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



static MyWindow create_tooltip(MirConnection* const connection, MirWindow* const parent, const MirPixelFormat format)
{
    MirRectangle zone = { 0, 0, 10, 10 };
    int const width = 50;
    int const height = 20;

    MirRenderSurface *const surface = mir_connection_create_render_surface_sync(connection, width, height);

    MirWindowSpec* const spec = mir_create_tip_window_spec(
        connection, width, height, parent, &zone, mir_edge_attachment_vertical);
    mir_window_spec_set_name(spec, "tooltip");
    mir_window_spec_set_min_width(spec, width);
    mir_window_spec_set_max_width(spec, width);
    mir_window_spec_set_min_height(spec, height);
    mir_window_spec_set_max_height(spec, height);
    mir_window_spec_add_render_surface(spec, surface, width, height, 0, 0);

    MirWindow* tooltip = mir_create_window_sync(spec);
    mir_window_spec_release(spec);

    MirBufferStream* bs = mir_render_surface_get_buffer_stream(surface, width, height, format);
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

    MyWindow result = {surface, tooltip};
    return result;
}
