
#include "mir_client/mir_client_library.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>

static const char servername[] = "/tmp/mir_socket";
static const char appname[] = "Dunnoyet";

static MirConnection *connection;
static EGLDisplay egldisplay;

#define CHECK(_cond, _err) \
    if (!(_cond)) \
    { \
        printf("%s\n", (_err)); \
        return __LINE__; \
    }

static void assign_result(void *result, void **arg)
{
    *arg = result;
}

static void shutdown(int signum)
{
    printf("Signal %d received. Good night.\n", signum);
    eglTerminate(egldisplay);
    mir_connection_release(connection);
    exit(0);
}

int main(int argc, char *argv[])
{
    EGLint attribs[] =
    {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_NONE
    };
    MirSurfaceParameters surfaceparm =
    {
        "Fred",
        256, 256,
        mir_pixel_format_xbgr_8888,
        mir_buffer_usage_hardware
    };
    MirDisplayInfo dinfo;
    MirSurface *surface;
    EGLConfig eglconfig;
    EGLint neglconfigs;
    EGLSurface eglsurface;
    EGLContext eglctx;

    (void)argc;
    (void)argv;

    mir_wait_for(mir_connect(servername, appname,
                             (mir_connected_callback)assign_result,
                             &connection));
    CHECK(mir_connection_is_valid(connection), "Can't get connection");

    mir_connection_get_display_info(connection, &dinfo);

    printf("Connected to display %s: %dx%d, supports %d pixel formats\n",
           servername, dinfo.width, dinfo.height,
           dinfo.supported_pixel_format_items);

    surfaceparm.pixel_format = dinfo.supported_pixel_format[0];
    printf("Using pixel format #%d\n", surfaceparm.pixel_format);

    mir_wait_for(mir_surface_create(connection, &surfaceparm,
                         (mir_surface_lifecycle_callback)assign_result,
                         &surface));
    CHECK(mir_surface_is_valid(surface), "Can't create a surface");

    egldisplay = eglGetDisplay(
                    mir_connection_get_egl_native_display(connection));
    CHECK(egldisplay != EGL_NO_DISPLAY, "Can't eglGetDisplay");

    mir_surface_get_egl_native_window(surface);
    
    CHECK(eglInitialize(egldisplay, NULL, NULL), "Can't eglInitialize");

    CHECK(eglChooseConfig(egldisplay, attribs, &eglconfig, 1, &neglconfigs),
          "Could not eglChooseConfig");
    CHECK(neglconfigs > 0, "No EGL config available");

    eglsurface = eglCreateWindowSurface(egldisplay, eglconfig,
            (EGLNativeWindowType)mir_surface_get_egl_native_window(surface),
            NULL);
    CHECK(eglsurface != EGL_NO_SURFACE, "eglCreateWindowSurface failed");

    eglctx = eglCreateContext(egldisplay, eglconfig, EGL_NO_CONTEXT, NULL);
    CHECK(eglctx != EGL_NO_CONTEXT, "eglCreateContext failed");

    CHECK(eglMakeCurrent(egldisplay, eglsurface, eglsurface, eglctx),
          "Can't eglMakeCurrent");

    signal(SIGINT, shutdown);
    signal(SIGTERM, shutdown);

    while (1)
    {
        glClearColor(1.0f, 0.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        sleep(1);
        eglSwapBuffers(egldisplay, eglsurface);
        glClearColor(1.0f, 1.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        sleep(1);
        eglSwapBuffers(egldisplay, eglsurface);
    }

    return 0;
}
