/*
 * Copyright © 2015-2016 Canonical Ltd.
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
 */

#include "eglapp.h"
#include "camapp.h"
#include <assert.h>
#include <stdio.h>
#include <math.h>
#include <GLES2/gl2.h>
#include <mir_toolkit/mir_surface.h>
#include <pthread.h>
#include <stdlib.h>
#include <signal.h>

typedef struct
{
    pthread_mutex_t mutex;
    bool resized;
} State;

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

static void on_event(MirSurface *surface, const MirEvent *event, void *context)
{
    (void)surface;
    State *state = (State*)context;

    // FIXME: We presently need to know that events come in on a different
    //        thread to main (LP: #1194384). When that's resolved, simple
    //        single-threaded apps like this won't need pthread.
    pthread_mutex_lock(&state->mutex);

    switch (mir_event_get_type(event))
    {
    case mir_event_type_input:
        break;
    case mir_event_type_resize:
        state->resized = true;
        break;
    case mir_event_type_close_surface:
        // TODO: eglapp.h needs a quit() function or different behaviour of
        //       mir_eglapp_shutdown().
        raise(SIGTERM);  // handled by eglapp
        break;
    default:
        break;
    }

    pthread_mutex_unlock(&state->mutex);
}

int main(int argc, char *argv[])
{
    const char vshadersrc[] =
        "attribute vec2 position;\n"
        "attribute vec2 texcoord;\n"
        "uniform mat4 projection;\n"
        "varying vec2 v_texcoord;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    gl_Position = projection *\n"
        "                  vec4(position, 0.0, 1.0);\n"
        "    v_texcoord = texcoord;\n"
        "}\n";

    const char raw_fshadersrc[] =
        "precision mediump float;\n"
        "varying vec2 v_texcoord;\n"
        "uniform sampler2D texture;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 f = texture2D(texture, v_texcoord);\n"
        "    gl_FragColor = vec4(f.rgb, 1.0);\n"
        "}\n";

    const char * const yuyv_greyscale_fshadersrc = raw_fshadersrc;

    // This is the Android YUV to RGB calculation.
    // TODO: Vary the shader to match the camera's reported colour space
    const char yuyv_quickcolour_fshadersrc[] =
        "precision mediump float;\n"
        "varying vec2 v_texcoord;\n"
        "uniform sampler2D texture;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 f = texture2D(texture, v_texcoord);\n"
        "    float y = (f.r + f.b) / 2.0;\n"  // Y unsigned (from two pixels)
        "    float u = f.g - 0.5;\n"       // U signed (same for both pixels)
        "    float v = f.a - 0.5;\n"       // V signed (same for both pixels)
        "    float r = clamp(y + 1.370705*v, 0.0, 1.0);\n"
        "    float g = clamp(y - 0.698001*v - 0.337633*u, 0.0, 1.0);\n"
        "    float b = clamp(y + 1.732446*u, 0.0, 1.0);\n"
        "    gl_FragColor = vec4(r, g, b, 1.0);\n"
        "}\n";

    // TODO: Selectable between high-res grey vs half-res colour?
    const char * const fshadersrc = yuyv_quickcolour_fshadersrc;

    Camera *cam = open_camera("/dev/video0", camera_pref_resolution, 1);
    if (!cam)
    {
        fprintf(stderr, "Failed to set up camera device\n");
        return 0;
    }

    unsigned int win_width = cam->pix.width;
    unsigned int win_height = cam->pix.height;
    if (!mir_eglapp_init(argc, argv, &win_width, &win_height))
        return 1;

    GLuint vshader = load_shader(vshadersrc, GL_VERTEX_SHADER);
    assert(vshader);
    GLuint fshader = load_shader(fshadersrc, GL_FRAGMENT_SHADER);
    assert(fshader);
    GLuint prog = glCreateProgram();
    assert(prog);
    glAttachShader(prog, vshader);
    glAttachShader(prog, fshader);
    glLinkProgram(prog);

    GLint linked;
    glGetProgramiv(prog, GL_LINK_STATUS, &linked);
    if (!linked)
    {
        GLchar log[1024];
        glGetProgramInfoLog(prog, sizeof log - 1, NULL, log);
        log[sizeof log - 1] = '\0';
        printf("Link failed: %s\n", log);
        return 2;
    }

    glUseProgram(prog);

    const GLfloat camw = cam->pix.width, camh = cam->pix.height;
    const GLfloat box[] =
    { // position   texcoord
        0.0f, camh, 0.0f, 1.0f,
        camw, camh, 1.0f, 1.0f,
        camw, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f,
    };
    GLint position = glGetAttribLocation(prog, "position");
    GLint texcoord = glGetAttribLocation(prog, "texcoord");
    glVertexAttribPointer(position, 2, GL_FLOAT, GL_FALSE, 4*sizeof(GLfloat),
                          box);
    glVertexAttribPointer(texcoord, 2, GL_FLOAT, GL_FALSE, 4*sizeof(GLfloat),
                          box+2);
    glEnableVertexAttribArray(position);
    glEnableVertexAttribArray(texcoord);

    GLint projection = glGetUniformLocation(prog, "projection");

    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glViewport(0, 0, win_width, win_height);

    State state =
    {
        PTHREAD_MUTEX_INITIALIZER,
        true
    };
    MirSurface *surface = mir_eglapp_native_surface();
    mir_surface_set_event_handler(surface, on_event, &state);

    bool first_frame = true;
    while (mir_eglapp_running())
    {
        bool wait_for_new_frame = true;
        pthread_mutex_lock(&state.mutex);

        if (state.resized)
        {
            // mir_eglapp_swap_buffers updates the viewport for us...
            GLint viewport[4];
            glGetIntegerv(GL_VIEWPORT, viewport);
            GLfloat scalex = 2.0f / viewport[2];
            GLfloat scaley = -2.0f / viewport[3];

            // Expand image to fit:
            GLfloat scalew = (GLfloat)viewport[2] / cam->pix.width;
            GLfloat scaleh = (GLfloat)viewport[3] / cam->pix.height;

            GLfloat scale;
            GLfloat offsetx = -1.0f, offsety = 1.0f;
            if (scalew <= scaleh)
            {
                scale = scalew;
                offsety -= (GLfloat)(viewport[3] - scale*cam->pix.height) /
                           viewport[3];
            }
            else
            {
                scale = scaleh;
                offsetx += (GLfloat)(viewport[2] - scale*cam->pix.width) /
                           viewport[2];
            }

            scalex *= scale;
            scaley *= scale;

            // TRANSPOSED projection matrix to convert from the Mir input
            // rectangle {{0,0},{w,h}} to GL screen rectangle {{-1,1},{2,2}}.
            GLfloat matrix[16] = {scalex, 0.0f,   0.0f, 0.0f,
                                  0.0f,   scaley, 0.0f, 0.0f,
                                  0.0f,   0.0f,   1.0f, 0.0f,
                                  offsetx,offsety,0.0f, 1.0f};

            // Note GL_FALSE: GLES does not support the transpose option
            glUniformMatrix4fv(projection, 1, GL_FALSE, matrix);
            state.resized = false;
            wait_for_new_frame = first_frame;
            first_frame = false;
        }

        if (wait_for_new_frame || frame_ready(cam))
        {
            const Buffer *buf = acquire_frame(cam);
            if (cam->pix.pixelformat == V4L2_PIX_FMT_YUYV)
            {
                if (fshadersrc == yuyv_greyscale_fshadersrc)
                {
                    // Greyscale, full resolution:
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA,
                                 cam->pix.width, cam->pix.height, 0,
                                 GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE,
                                 buf->start);
                }
                else if (fshadersrc == yuyv_quickcolour_fshadersrc)
                {
                    // Colour, half resolution:
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                                 cam->pix.width/2, cam->pix.height, 0,
                                 GL_RGBA, GL_UNSIGNED_BYTE,
                                 buf->start);
                }
                // TODO: Colour, full resolution. But it will be slow :(
            }
            else
            {
                char str[5];
                fourcc_string(cam->pix.pixelformat, str);
                fprintf(stderr, "FIXME: Unsupported camera pixel format 0x%08lx: %s\n",
                        (long)cam->pix.pixelformat, str);
            }
            release_frame(cam, buf);
        }

        glClear(GL_COLOR_BUFFER_BIT);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

        pthread_mutex_unlock(&state.mutex);

        mir_eglapp_swap_buffers();
    }

    mir_surface_set_event_handler(surface, NULL, NULL);
    mir_eglapp_shutdown();
    close_camera(cam);

    return 0;
}
