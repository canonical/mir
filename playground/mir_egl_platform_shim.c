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
#include "mir_toolkit/mir_extension_core.h"
#include "mir_toolkit/extensions/android_egl.h"
#include "mir_toolkit/extensions/hardware_buffer_stream.h"
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <stdlib.h>
#include <string.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
//Information the driver will have to maintain
typedef struct
{
    MirConnection* connection;      //EGLNativeDisplayType
    MirRenderSurface* surface;      //EGLNativeWindowType
    MirBufferStream* stream;        //the internal semantics a driver might want to use...
                                    //could be MirPresentationChain as well
    int current_physical_width;     //The driver is in charge of the physical width
    int current_physical_height;    //The driver is in charge of the physical height

    MirExtensionAndroidEGLV1 const* ext;
    PFNEGLCREATEIMAGEKHRPROC eglCreateImageKHR;
    PFNEGLDESTROYIMAGEKHRPROC eglDestroyImageKHR;
    PFNGLEGLIMAGETARGETTEXTURE2DOESPROC glEGLImageTargetTexture2DOES; 
 
} DriverInfo;
static DriverInfo* info = NULL;

typedef struct
{
    struct ANativeWindowBuffer *buffer;    
    EGLImageKHR img;
} ShimEGLImageKHR;

EGLSurface future_driver_eglCreateWindowSurface(
    EGLDisplay display, EGLConfig config, MirRenderSurface* surface, const EGLint* attr)
{
    MirExtensionHardwareBufferStreamV1 const * ext = mir_extension_hardware_buffer_stream_v1(info->connection);
    if (info->surface || !ext)
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
    info->stream = ext->get_hardware_buffer_stream(surface,
        info->current_physical_width,
        info->current_physical_height,
        pixel_format);

    printf("The driver chose pixel format %d.\n", pixel_format);
    return eglCreateWindowSurface(display, config, (EGLNativeWindowType) surface, attr);
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
#pragma GCC diagnostic pop

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

    info->ext = mir_extension_android_egl_v1(info->connection);
    info->eglCreateImageKHR = (PFNEGLCREATEIMAGEKHRPROC) eglGetProcAddress("eglCreateImageKHR");
    info->eglDestroyImageKHR = (PFNEGLDESTROYIMAGEKHRPROC) eglGetProcAddress("eglDestroyImageKHR");
    info->glEGLImageTargetTexture2DOES =
        (PFNGLEGLIMAGETARGETTEXTURE2DOESPROC) eglGetProcAddress("glEGLImageTargetTexture2DOES");
    return eglGetDisplay(mir_connection_get_egl_native_display(connection));
}

EGLBoolean future_driver_eglTerminate(EGLDisplay display)
{
    if (info)
        free(info);
    return eglTerminate(display);
}

EGLImageKHR future_driver_eglCreateImageKHR(
    EGLDisplay dpy, EGLContext ctx, EGLenum target, EGLClientBuffer buffer, const EGLint *attrib_list)
{
    //bit pedantic, but we should validate the parameters we require from the extension
    if ( (target != EGL_NATIVE_PIXMAP_KHR) || (ctx != EGL_NO_CONTEXT) || !info || !info->ext )
        return EGL_NO_IMAGE_KHR;

    //check we have subloaded extension available.
    if(!strstr(eglQueryString(dpy, EGL_EXTENSIONS), "EGL_ANDROID_image_native_buffer"))
        return EGL_NO_IMAGE_KHR;

    static EGLint const expected_attrs[] = { EGL_IMAGE_PRESERVED_KHR, EGL_TRUE, EGL_NONE };
    int i = 0;
    while ( (attrib_list[i] != EGL_NONE) && (expected_attrs[i] != EGL_NONE) )
    {
        if (attrib_list[i] != expected_attrs[i])
            return EGL_NO_IMAGE_KHR;
        i++;
    }

    ShimEGLImageKHR* img = (ShimEGLImageKHR*) malloc(sizeof(ShimEGLImageKHR));
    img->buffer = info->ext->create_buffer(buffer);
    img->img = info->eglCreateImageKHR(dpy, EGL_NO_CONTEXT, EGL_NATIVE_BUFFER_ANDROID,
        img->buffer, attrib_list);
    return (EGLImageKHR) img;
}

EGLBoolean future_driver_eglDestroyImageKHR (EGLDisplay dpy, EGLImageKHR image)
{
    if (!info)
        return EGL_FALSE;
    ShimEGLImageKHR* img = (ShimEGLImageKHR*) image;

    EGLBoolean rc = info->eglDestroyImageKHR(dpy, image);
    info->ext->destroy_buffer(img->buffer);
    free(img);
    return rc;
}

void future_driver_glEGLImageTargetTexture2DOES (GLenum target, GLeglImageOES image)
{
    if (!info)
        return;
    ShimEGLImageKHR* img = (ShimEGLImageKHR*) image;
    info->glEGLImageTargetTexture2DOES(target, img->img);
}
