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
 * Author: Daniel van Vugt <daniel.van.vugt@canonical.com>
 *         Cemil Azizoglu <cemil.azizoglu@canonical.com>
 *         Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir_toolkit/mir_client_library.h"
#include "mir_toolkit/rs/mir_render_surface.h"
#include "mir_toolkit/mir_buffer.h"
#include "mir_toolkit/mir_presentation_chain.h"
#include "mir_egl_platform_shim.h"
#include "diamond.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <pthread.h>

static volatile sig_atomic_t running = 0;
static void shutdown(int signum)
{
    if (running)
    {
        running = 0;
        printf("Signal %d received. Good night.\n", signum);
    }
}

#define CHECK(_cond, _err) \
    if (!(_cond)) \
    { \
       printf("%s\n", (_err)); \
       return -1; \
    }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
//The client arranges the scene in the subscene
void resize_callback(MirWindow* window, MirEvent const* event, void* context)
{
    (void) window;
    MirEventType type = mir_event_get_type(event);
    if (type == mir_event_type_resize)
    {
        MirResizeEvent const* resize_event = mir_event_get_resize_event(event);
        int width = mir_resize_event_get_width(resize_event);
        int height = mir_resize_event_get_height(resize_event);
        MirRenderSurface* rs = (MirRenderSurface*) context;
        mir_render_surface_set_size(rs, width, height);
    }
}

typedef struct
{
    pthread_mutex_t* mut;
    pthread_cond_t* cond;
    MirBuffer** buffer;
} BufferWait;

void wait_buffer(MirBuffer* b, void* context)
{
    BufferWait* w = (BufferWait*) context;
    pthread_mutex_lock(w->mut);
    *w->buffer = b;
    pthread_cond_broadcast(w->cond);
    pthread_mutex_unlock(w->mut);
}

bool fill_buffer(MirBuffer* buffer)
{
    MirBufferLayout layout = mir_buffer_layout_unknown;
    MirGraphicsRegion region;
    bool rc = mir_buffer_map(buffer, &region, &layout);
    if (!rc || layout == mir_buffer_layout_unknown)
        return false;

    unsigned int *data = (unsigned int*) region.vaddr;
    for (int i = 0; i < region.width; i++)
    {
        for (int j = 0; j < region.height; j++)
        {
            int idx = (i * (region.stride/4)) + j;
            if (idx % 32 > 16)
                data[ idx ] = 0xFF00FFFF;
            else
                data[ idx ] = 0xFFFF0000;
        }
    }
    mir_buffer_unmap(buffer);
    return true;
}

int main(int argc, char *argv[])
{
    //once full transition to Mir platform has been made, internal shim will be removed,
    //and the examples/ will use MirConnection/MirRenderSurface/MirBuffer as their egl types.
    int use_shim = 1;
    int swapinterval = 1;
    char* socket = NULL; 
    int c;
    while ((c = getopt(argc, argv, "ehnm:")) != -1)
    {
        switch (c)
        {
            case 'm':
                socket = optarg;
                break;
            case 'e':
                use_shim = 0;
                break;
            case 'n':
                swapinterval = 0;
                break;
            case 'h':
            default:
                printf(
                    "Usage:\n"
                    "\t-m mir_socket\n"
                    "\t-e use egl library directly, instead of using shim\n"
                    "\t-n use swapinterval 0\n"
                    "\t-h this message\n");
                return -1;
        }
    }

    const char* appname = "EGL Render Surface Demo";
    int width = 300;
    int height = 300;
    EGLDisplay egldisplay;
    EGLSurface eglsurface;
    EGLint ctxattribs[] =
    {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };
    EGLContext eglctx;
    EGLConfig eglconfig;
    EGLint neglconfigs;
    EGLBoolean ok;
    MirConnection* connection = NULL;
    MirWindow* window = NULL;
    MirRenderSurface* render_surface = NULL;

    signal(SIGINT, shutdown);
    signal(SIGTERM, shutdown);
    signal(SIGHUP, shutdown);

    if (use_shim)
        printf("internal EGL driver shim in use\n");
    else
        printf("using EGL driver directly\n");

    connection = mir_connect_sync(socket, appname);
    CHECK(mir_connection_is_valid(connection), "Can't get connection");

    BufferWait w; 
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
    MirBuffer* buffer = NULL;
    w.mut = &mutex;
    w.cond = &cond;
    w.buffer = &buffer;

    mir_connection_allocate_buffer(
        connection, 256, 256, mir_pixel_format_abgr_8888, wait_buffer, &w);

    pthread_mutex_lock(&mutex);
    while (buffer == NULL)
        pthread_cond_wait(&cond, &mutex);
    pthread_mutex_unlock(&mutex);

    bool const filled = fill_buffer(buffer);

    if (use_shim)
        egldisplay = future_driver_eglGetDisplay(connection);
    else
        egldisplay = eglGetDisplay(connection);

    CHECK(egldisplay != EGL_NO_DISPLAY, "Can't eglGetDisplay");

    int maj =0;
    int min = 0;
    ok = eglInitialize(egldisplay, &maj, &min);
    CHECK(ok, "Can't eglInitialize");
    printf("EGL version %i.%i\n", maj, min);

    const EGLint attribs[] =
    {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_NONE
    };

    ok = eglChooseConfig(egldisplay, attribs, &eglconfig, 1, &neglconfigs);
    CHECK(ok, "Could not eglChooseConfig");
    CHECK(neglconfigs > 0, "No EGL config available");

    render_surface = mir_connection_create_render_surface_sync(connection, width, height);
    CHECK(mir_render_surface_is_valid(render_surface), "could not create render surface");
    CHECK(mir_render_surface_get_error_message(render_surface), "");

    if (use_shim)
        eglsurface = future_driver_eglCreateWindowSurface(egldisplay, eglconfig, render_surface, NULL);
    else
        eglsurface = eglCreateWindowSurface(egldisplay, eglconfig, (EGLNativeWindowType) render_surface, NULL);
    if (eglsurface == EGL_NO_SURFACE)
    {
        printf("eglCreateWindowSurface failed. "
               "This is likely because the egl driver does not support the usage of MirRenderSurface\n");
        mir_render_surface_release(render_surface);
        mir_connection_release(connection);   
        eglTerminate(egldisplay);
        return 0;
    }


    //The format field is only used for default-created streams.
    //width and height are the logical width the user wants the window to be
    MirWindowSpec *spec =
        mir_create_normal_window_spec(connection, width, height);

    CHECK(spec, "Can't create a window spec");
    mir_window_spec_set_name(spec, appname);
    mir_window_spec_add_render_surface(spec, render_surface, width, height, 0, 0);

    mir_window_spec_set_event_handler(spec, resize_callback, render_surface);

    window = mir_create_window_sync(spec);
    mir_window_spec_release(spec);

    eglctx = eglCreateContext(egldisplay, eglconfig, EGL_NO_CONTEXT,
                              ctxattribs);
    CHECK(eglctx != EGL_NO_CONTEXT, "eglCreateContext failed");

    ok = eglMakeCurrent(egldisplay, eglsurface, eglsurface, eglctx);
    CHECK(ok, "Can't eglMakeCurrent");

    eglSwapInterval(egldisplay, swapinterval);
    EGLImageKHR image = EGL_NO_IMAGE_KHR;
    PFNEGLCREATEIMAGEKHRPROC eglCreateImageKHR = NULL;
    PFNEGLDESTROYIMAGEKHRPROC eglDestroyImageKHR = NULL;
    char const* extensions = eglQueryString(egldisplay, EGL_EXTENSIONS);
    printf("EGL extensions %s\n", extensions);
    if (strstr(extensions, "EGL_KHR_image_pixmap"))
    {
        static EGLint const image_attrs[] = { EGL_IMAGE_PRESERVED_KHR, EGL_TRUE, EGL_NONE };

        if (use_shim)
        {
            eglCreateImageKHR = future_driver_eglCreateImageKHR; 
            eglDestroyImageKHR = future_driver_eglDestroyImageKHR; 
        }
        else
        {
            eglCreateImageKHR = (PFNEGLCREATEIMAGEKHRPROC) eglGetProcAddress("eglCreateImageKHR"); 
            eglDestroyImageKHR = (PFNEGLDESTROYIMAGEKHRPROC) eglGetProcAddress("eglDestroyImageKHR"); 
        }
        if (filled)
            image = eglCreateImageKHR(egldisplay, EGL_NO_CONTEXT, EGL_NATIVE_PIXMAP_KHR, buffer, image_attrs);
    }

    Diamond diamond;
    if (image == EGL_NO_IMAGE_KHR)
    {
        printf("MirBuffer import not supported by driver. Should see red/white checker\n");
        diamond = setup_diamond();
    }
    else
    {
        printf("MirBuffer import supported by driver. Should see yellow/blue stripes\n");
        diamond = setup_diamond_import(image, use_shim);
    }

    EGLint viewport_width = -1;
    EGLint viewport_height = -1;
    running = 1;
    while (running)
    {
        eglQuerySurface(egldisplay, eglsurface, EGL_WIDTH, &viewport_width);
        eglQuerySurface(egldisplay, eglsurface, EGL_HEIGHT, &viewport_height);
        render_diamond(&diamond, viewport_width, viewport_height);
        if (use_shim)
            future_driver_eglSwapBuffers(egldisplay, eglsurface);
        else
            eglSwapBuffers(egldisplay, eglsurface);
    }

    destroy_diamond(&diamond);
    eglMakeCurrent(egldisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    if (image)
        eglDestroyImageKHR(egldisplay, image);

    if (use_shim) 
        future_driver_eglTerminate(egldisplay);
    else
        eglTerminate(egldisplay);
    mir_render_surface_release(render_surface);
    mir_window_release_sync(window);
    mir_connection_release(connection);
    return 0;
}
#pragma GCC diagnostic pop
