
#include "mir_client/mir_client_library.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <assert.h>
#include <unistd.h> /* sleep() */
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
        return EGL_FALSE; \
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

EGLBoolean mir_egl_app_init(int *width, int *height,
                            EGLDisplay *disp, EGLSurface *win)
{
    EGLint attribs[] =
    {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_COLOR_BUFFER_TYPE, EGL_RGB_BUFFER,
        EGL_NONE
    };
    EGLint ctxattribs[] =
    {
        EGL_CONTEXT_CLIENT_VERSION, 2,
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
    EGLBoolean ok;

    mir_wait_for(mir_connect(servername, appname,
                             (mir_connected_callback)assign_result,
                             &connection));
    CHECK(mir_connection_is_valid(connection), "Can't get connection");

    mir_connection_get_display_info(connection, &dinfo);

    printf("Connected to display %s: %dx%d, supports %d pixel formats\n",
           servername, dinfo.width, dinfo.height,
           dinfo.supported_pixel_format_items);

    surfaceparm.width = *width > 0 ? *width : dinfo.width;
    surfaceparm.height = *height > 0 ? *height : dinfo.height;
    surfaceparm.pixel_format = dinfo.supported_pixel_format[0];
    printf("Using pixel format #%d\n", surfaceparm.pixel_format);

    mir_wait_for(mir_surface_create(connection, &surfaceparm,
                         (mir_surface_lifecycle_callback)assign_result,
                         &surface));
    CHECK(mir_surface_is_valid(surface), "Can't create a surface");

    egldisplay = eglGetDisplay(
                    mir_connection_get_egl_native_display(connection));
    CHECK(egldisplay != EGL_NO_DISPLAY, "Can't eglGetDisplay");

    ok = eglInitialize(egldisplay, NULL, NULL);
    CHECK(ok, "Can't eglInitialize");

    ok = eglChooseConfig(egldisplay, attribs, &eglconfig, 1, &neglconfigs);
    CHECK(ok, "Could not eglChooseConfig");
    CHECK(neglconfigs > 0, "No EGL config available");

    eglsurface = eglCreateWindowSurface(egldisplay, eglconfig,
            (EGLNativeWindowType)mir_surface_get_egl_native_window(surface),
            NULL);
    CHECK(eglsurface != EGL_NO_SURFACE, "eglCreateWindowSurface failed");

    eglctx = eglCreateContext(egldisplay, eglconfig, EGL_NO_CONTEXT, ctxattribs);
    CHECK(eglctx != EGL_NO_CONTEXT, "eglCreateContext failed");

    ok = eglMakeCurrent(egldisplay, eglsurface, eglsurface, eglctx);
    CHECK(ok, "Can't eglMakeCurrent");

    signal(SIGINT, shutdown);
    signal(SIGTERM, shutdown);

    *disp = egldisplay;
    *win = eglsurface;
    *width = surfaceparm.width;
    *height = surfaceparm.height;

    return EGL_TRUE;
}

static GLuint LoadShader(const char *src, GLenum type)
{
    GLuint shader = glCreateShader(type);
    if (shader)
    {
        GLint compiled;
        glShaderSource(shader, 1, &src, NULL);
        glCompileShader(shader);
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
        if (!compiled)
        {
            GLchar log[1024];
            glGetShaderInfoLog(shader, sizeof log - 1, NULL, log);
            log[sizeof log - 1] = '\0';
            printf("LoadShader compile failed: %s\n", log);
            glDeleteShader(shader);
            shader = 0;
        }
    }
    return shader;
}

/* Colours from http://design.ubuntu.com/brand/colour-palette */
#define MID_AUBERGINE 0.368627451f, 0.152941176f, 0.31372549f
#define ORANGE        0.866666667f, 0.282352941f, 0.141414141f

int main(int argc, char *argv[])
{
    EGLDisplay disp;
    EGLSurface surf;

    const char vshadersrc[] =
        "attribute vec4 vPosition;    \n"
        "void main()                  \n"
        "{                            \n"
        "    gl_Position = vPosition; \n"
        "}                            \n";

    const char fshadersrc[] =
        "precision mediump float;                        \n"
        "void main()                                     \n"
        "{                                               \n"
        "    gl_FragColor = vec4(0.866666667, 0.282352941, 0.141414141, 1.0);"
        "}                                               \n";

    const GLfloat vertices[] =
    {
        0.0f, 1.0f, 0.0f,
       -1.0f,-0.866f, 0.0f,
        1.0f,-0.866f, 0.0f
    };
    GLuint vshader, fshader, prog;
    GLint linked;
    int width = 0, height = 0;

    (void)argc;
    (void)argv;

    if (!mir_egl_app_init(&width, &height, &disp, &surf))
    {
        printf("Can't initialize EGL\n");
        return 1;
    }

    vshader = LoadShader(vshadersrc, GL_VERTEX_SHADER);
    assert(vshader);
    fshader = LoadShader(fshadersrc, GL_FRAGMENT_SHADER);
    assert(fshader);
    prog = glCreateProgram();
    assert(prog);
    glAttachShader(prog, vshader);
    glAttachShader(prog, fshader);
    glBindAttribLocation(prog, 0, "vPosition");
    glLinkProgram(prog);

    glGetProgramiv(prog, GL_LINK_STATUS, &linked);
    if (!linked)
    {
        GLchar log[1024];
        glGetProgramInfoLog(prog, sizeof log - 1, NULL, log);
        log[sizeof log - 1] = '\0';
        printf("Link failed: %s\n", log);
        return 2;
    }

    glClearColor(MID_AUBERGINE, 1.0);
    glViewport(0, 0, width, height);
    glUseProgram(prog);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, vertices);
    glEnableVertexAttribArray(0);

    while (1)
    {
        glClear(GL_COLOR_BUFFER_BIT);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        eglSwapBuffers(disp, surf);
        sleep(1);
    }

    return 0;
}
