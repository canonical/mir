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

#include "mir_toolkit/mir_client_library.h"
#include "mir_toolkit/mir_client_library_tps.h"

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>
#include <sys/wait.h>
#include "eglapp.h"
#include <GLES2/gl2.h>

// ///\internal [MirDemoState_tag]
// // Utility structure for the state of a single surface session.
// typedef struct MirDemoState
// {
//     MirConnection *connection;
//     MirSurface *surface;
// } MirDemoState;
// ///\internal [MirDemoState_tag]

// ///\internal [Callback_tag]
// // Callback to update MirDemoState on connection
// static void connection_callback(MirConnection *new_connection, void *context)
// {
//     ((MirDemoState*)context)->connection = new_connection;
// }

// // Callback to update MirDemoState on surface_create
// static void surface_create_callback(MirSurface *new_surface, void *context)
// {
//     ((MirDemoState*)context)->surface = new_surface;
// }

// // Callback to update MirDemoState on surface_release
// static void surface_release_callback(MirSurface *old_surface, void *context)
// {
//     (void)old_surface;
//     ((MirDemoState*)context)->surface = 0;
// }
// ///\internal [Callback_tag]

// void start_session(const char* server, const char* name, MirDemoState* mcd)
// {
//     ///\internal [connect_tag]
//     // Call mir_connect and wait for callback to complete.
//     mir_wait_for(mir_connect(server, name, connection_callback, mcd));
//     puts("Connected for 'demo_client_app2'");
//     ///\internal [connect_tag]

//     // We expect a connection handle;
//     // we expect it to be valid; and,
//     // we don't expect an error description
//     assert(mcd->connection != NULL);
//     assert(mir_connection_is_valid(mcd->connection));
//     assert(strcmp(mir_connection_get_error_message(mcd->connection), "") == 0);

//     // We can query information about the platform we're running on
//     {
//         MirPlatformPackage platform_package;
//         platform_package.data_items = -1;
//         platform_package.fd_items = -1;

//         mir_connection_get_platform(mcd->connection, &platform_package);
//         assert(0 <= platform_package.data_items);
//         assert(0 <= platform_package.fd_items);
//     }
// }

// void stop_session(MirDemoState* mcd)
// {
//     if (mcd->surface)
//     {
//         ///\internal [surface_release_tag]
//         // We should release our surface
//         mir_wait_for(mir_surface_release(mcd->surface, surface_release_callback, mcd));
//         puts("Surface released for 'demo_client_app2'");
//         ///\internal [surface_release_tag]
//     }

//     ///\internal [connection_release_tag]
//     // We should release our connection
//     mir_connection_release(mcd->connection);
//     puts("Connection released for 'demo_client_app2'");
//     ///\internal [connection_release_tag]
// }

// void demo_client_app2(const char* server)
// {
//     MirDemoState mcd;
//     mcd.connection = 0;
//     mcd.surface = 0;
//     start_session(server, "demo_client_trusted_session_app2", &mcd);

//     // Identify a supported pixel format
//     MirPixelFormat pixel_format;
//     unsigned int valid_formats;
//     mir_connection_get_available_surface_formats(mcd.connection, &pixel_format, 1, &valid_formats);
//     MirSurfaceParameters const request_params =
//         {__PRETTY_FUNCTION__, 640, 480, pixel_format,
//          mir_buffer_usage_hardware, mir_display_output_id_invalid};

//     ///\internal [surface_create_tag]
//     // ...we create a surface using that format and wait for callback to complete.
//     mir_wait_for(mir_connection_create_surface(mcd.connection, &request_params, surface_create_callback, &mcd));
//     puts("Surface created for 'demo_client_app2'");
//     ///\internal [surface_create_tag]

//     // We expect a surface handle;
//     // we expect it to be valid; and,
//     // we don't expect an error description
//     assert(mcd.surface != NULL);
//     assert(mir_surface_is_valid(mcd.surface));
//     assert(strcmp(mir_surface_get_error_message(mcd.surface), "") == 0);

//     // Wait for stdin
//     char buff[1];
//     read(STDIN_FILENO, &buff, 1);
//     puts("App2 Done");

//     stop_session(&mcd);
// }

// // The main() function deals with parsing arguments and defaults
// int main(int argc, char* argv[])
// {
//     // Some variables for holding command line options
//     char const *server = NULL;

//     // Parse the command line
//     {
//         int arg;
//         opterr = 0;
//         while ((arg = getopt (argc, argv, "c:hm:")) != -1)
//         {
//             switch (arg)
//             {
//             case 'm':
//                 server = optarg;
//                 break;

//             case '?':
//             case 'h':
//             default:
//                 puts(argv[0]);
//                 puts("Usage:");
//                 puts("    -m <Mir server socket>");
//                 puts("    -h: this help text");
//                 return -1;
//             }
//         }
//     }

//     printf("mir_demo_client_trusted_session_app2: %d\n", getpid());

//     demo_client_app2(server);

//     return 0;
// }


static GLuint load_shader(const char *src, GLenum type)
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
            printf("load_shader compile failed: %s\n", log);
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
    const char vshadersrc[] =
        "attribute vec4 vPosition;            \n"
        "uniform float theta;                 \n"
        "void main()                          \n"
        "{                                    \n"
        "    float c = cos(theta);            \n"
        "    float s = sin(theta);            \n"
        "    mat2 m;                          \n"
        "    m[0] = vec2(c, s);               \n"
        "    m[1] = vec2(-s, c);              \n"
        "    vec2 p = m * vec2(vPosition);    \n"
        "    gl_Position = vec4(p, 0.0, 1.0); \n"
        "}                                    \n";

    const char fshadersrc[] =
        "precision mediump float;             \n"
        "uniform vec4 col;                    \n"
        "void main()                          \n"
        "{                                    \n"
        "    gl_FragColor = col;              \n"
        "}                                    \n";

    const GLfloat vertices[] =
    {
        0.0f, 1.0f,
       -1.0f,-0.866f,
        1.0f,-0.866f,
    };
    GLuint vshader, fshader, prog;
    GLint linked, col, vpos, theta;
    unsigned int width = 768, height = 1280;
    GLfloat angle = 0.0f;

    if (!mir_eglapp_init(argc, argv, &width, &height))
        return 1;

    vshader = load_shader(vshadersrc, GL_VERTEX_SHADER);
    assert(vshader);
    fshader = load_shader(fshadersrc, GL_FRAGMENT_SHADER);
    assert(fshader);
    prog = glCreateProgram();
    assert(prog);
    glAttachShader(prog, vshader);
    glAttachShader(prog, fshader);
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

    glClearColor(MID_AUBERGINE, mir_eglapp_background_opacity);
    glViewport(0, 0, width, height);

    glUseProgram(prog);

    vpos = glGetAttribLocation(prog, "vPosition");
    col = glGetUniformLocation(prog, "col");
    theta = glGetUniformLocation(prog, "theta");
    glUniform4f(col, ORANGE, 1.0f);

    glVertexAttribPointer(vpos, 2, GL_FLOAT, GL_FALSE, 0, vertices);
    glEnableVertexAttribArray(0);

    while (mir_eglapp_running())
    {
        glClear(GL_COLOR_BUFFER_BIT);
        glUniform1f(theta, angle);
        angle += 0.02f;
        glDrawArrays(GL_TRIANGLES, 0, 3);
        mir_eglapp_swap_buffers();
    }

    mir_eglapp_shutdown();

    return 0;
}
