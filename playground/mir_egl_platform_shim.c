/*
 * Copyright © 2016 Canonical Ltd.
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
 * Author: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir_egl_platform_shim.h"
#include "mir_toolkit/mir_client_library.h"
#include <stdlib.h>
#include <string.h>

//Information the driver will have to maintain
typedef struct
{
    MirConnection* connection;      //EGLNativeDisplayType
    MirRenderSurface* surface;      //EGLNativeWindowType
    MirBufferStream* stream;        //the internal semantics a driver might want to use...
                                    //could be MirPresentationChain as well
    int current_physical_width;     //The driver is in charge of the physical width
    int current_physical_height;    //The driver is in charge of the physical height
} DriverInfo;
static DriverInfo* info = NULL;

EGLSurface future_driver_eglCreateWindowSurface(
    EGLDisplay display, EGLConfig config, MirRenderSurface* surface)
{
    if (info->surface)
    {
        printf("shim only supports one surface at the moment");
        return EGL_NO_SURFACE;
    }

    info->surface = surface;
    mir_render_surface_get_size(surface,
        &info->current_physical_width, &info->current_physical_height);

    //TODO: the driver needs to be selecting a pixel format that's acceptable based on
    //      the EGLConfig. mir_connection_get_egl_pixel_format
    //      needs to be deprecated once the drivers support the Mir EGL platform.
    MirPixelFormat pixel_format = mir_connection_get_egl_pixel_format(info->connection, display, config);
    //this particular [silly] driver has chosen the buffer stream as the way it wants to post
    //its hardware content. I'd think most drivers would want MirPresentationChain for flexibility
    info->stream =
            mir_render_surface_with_content_get_buffer_stream(surface,
                    info->current_physical_width, info->current_physical_height,
                                                                 pixel_format,
                                                                 mir_buffer_usage_hardware);

    printf("The driver chose pixel format %d.\n", pixel_format);
    return eglCreateWindowSurface(display, config, (EGLNativeWindowType) surface, NULL);
}

EGLBoolean future_driver_eglSwapBuffers(EGLDisplay display, EGLSurface surface)
{
    int width = -1;
    int height = -1;
    mir_render_surface_get_size(info->surface, &width, &height);
    if (width != info->current_physical_width || height != info->current_physical_height)
    {
        //note that this affects the next buffer that we get after swapbuffers.
        mir_buffer_stream_set_size(info->stream, width, height);
        info->current_physical_width = width;
        info->current_physical_height = height;
    } 
    return eglSwapBuffers(display, surface);
}

EGLDisplay future_driver_eglGetDisplay(MirConnection* connection)
{
    if (info)
    {
        printf("shim only supports one display connection at the moment");
        return EGL_NO_DISPLAY;
    }

    info = malloc(sizeof(DriverInfo));
    memset(info, 0, sizeof(*info));
    info->connection = connection;
    return eglGetDisplay(mir_connection_get_egl_native_display(connection));
}

EGLBoolean future_driver_eglTerminate(EGLDisplay display)
{
    if (info)
        free(info);
    return eglTerminate(display);
}
