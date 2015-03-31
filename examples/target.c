/*
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
 * Author: Daniel van Vugt <daniel.van.vugt@canonical.com>
 */

#include "eglapp.h"
#include <assert.h>
#include <stdio.h>
#include <math.h>
#include <GLES2/gl2.h>
#include <mir_toolkit/mir_surface.h>
#include <pthread.h>

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t change = PTHREAD_COND_INITIALIZER;

typedef struct
{
    float x, y;
} Vec2;

enum
{
    max_touches = 10
};

typedef struct
{
    bool running;
    bool resized;
    int touches;
    Vec2 touch[max_touches];
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

GLuint generate_target_texture()
{
    const int width = 512, height = width;
    typedef struct { GLubyte r, b, g, a; } Texel;
    Texel image[height][width];
    const int centrex = width/2, centrey = height/2;
    const Texel blank = {0, 0, 0, 0};
    const int radius = centrex - 1;
    const int rings = 7;
    const Texel ring[7] =
    {
        {  0,   0,   0, 255},
        {  0,   0, 255, 255},
        {  0, 255, 255, 255},
        {  0, 255,   0, 255},
        {255, 255,   0, 255},
        {255, 128,   0, 255},
        {255,   0,   0, 255},
    };

    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            int dx = x - centrex, dy = y - centrey;
            int layer = rings * sqrtf(dx * dx + dy * dy) / radius;
            image[y][x] = layer < rings ? ring[layer] : blank;
        }
    }

    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                                   GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, image);
    glGenerateMipmap(GL_TEXTURE_2D);

    return tex;
}

static void on_event(MirSurface *surface, const MirEvent *event, void *context)
{
    (void)surface;

    // FIXME: We presently need to know that events come in on a different
    //        thread to main (LP: #1194384). When that's resolved, simple
    //        single-threaded apps like this won't need pthreads.
    pthread_mutex_lock(&mutex);

    State *state = (State*)context;

    switch (mir_event_get_type(event))
    {
    case mir_event_type_input:
    {
        const MirInputEvent *input = mir_event_get_input_event(event);
        if (mir_input_event_get_type(input) == mir_input_event_type_pointer)
        {
            const MirPointerEvent *pointer =
                mir_input_event_get_pointer_event(input);
            state->touches = 1;
            state->touch[0].x = mir_pointer_event_axis_value(pointer,
                                                         mir_pointer_axis_x);
            state->touch[0].y = mir_pointer_event_axis_value(pointer,
                                                         mir_pointer_axis_y);
        }
        else if (mir_input_event_get_type(input) == mir_input_event_type_touch)
        {
            const MirTouchEvent *touch = mir_input_event_get_touch_event(input);
            int n = mir_touch_event_point_count(touch);
            if (n > max_touches)
                n = max_touches;
            state->touches = n;
            for (int t = 0; t < n; ++t)
            {
                state->touch[t].x = mir_touch_event_axis_value(touch, t,
                                                        mir_touch_axis_x);
                state->touch[t].y = mir_touch_event_axis_value(touch, t,
                                                        mir_touch_axis_y);
            }
        }
        break;
    }
    case mir_event_type_resize:
        state->resized = true;
        break;
    case mir_event_type_close_surface:
        state->running = false;
        break;
    default:
        break;
    }

    pthread_cond_signal(&change);
    pthread_mutex_unlock(&mutex);
}

int main(int argc, char *argv[])
{
    const char vshadersrc[] =
        "attribute vec2 position;\n"
        "attribute vec2 texcoord;\n"
        "uniform float scale;\n"
        "uniform vec2 translate;\n"
        "uniform mat4 projection;\n"
        "varying vec2 v_texcoord;\n"
        "void main()\n"
        "{\n"
        "    gl_Position = projection *\n"
        "                  vec4(position * scale + translate, 0.0, 1.0);\n"
        "    v_texcoord = texcoord;\n"
        "}\n";

    const char fshadersrc[] =
        "precision mediump float;\n"
        "varying vec2 v_texcoord;\n"
        "uniform sampler2D texture;\n"
        "void main()\n"
        "{\n"
        "    gl_FragColor = texture2D(texture, v_texcoord);\n"
        "}\n";

    GLuint vshader, fshader, prog;
    GLint linked;

    static unsigned int width = 0, height = 0;
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

    glUseProgram(prog);

    const GLfloat square[] =
    { // position      texcoord
        -0.5f, +0.5f,  0.0f, 1.0f,
        +0.5f, +0.5f,  1.0f, 1.0f,
        +0.5f, -0.5f,  1.0f, 0.0f,
        -0.5f, -0.5f,  0.0f, 0.0f,
    };
    GLint position = glGetAttribLocation(prog, "position");
    GLint texcoord = glGetAttribLocation(prog, "texcoord");
    glVertexAttribPointer(position, 2, GL_FLOAT, GL_FALSE, 4*sizeof(GLfloat),
                          square);
    glVertexAttribPointer(texcoord, 2, GL_FLOAT, GL_FALSE, 4*sizeof(GLfloat),
                          square+2);
    glEnableVertexAttribArray(position);
    glEnableVertexAttribArray(texcoord);

    GLint scale = glGetUniformLocation(prog, "scale");
    glUniform1f(scale, 256.0f);

    GLint translate = glGetUniformLocation(prog, "translate");
    glUniform2f(translate, 0.0f, 0.0f);

    GLint projection = glGetUniformLocation(prog, "projection");

    GLuint tex = generate_target_texture();
    glBindTexture(GL_TEXTURE_2D, tex);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glViewport(0, 0, width, height);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    MirSurface *surface = mir_eglapp_native_surface();

    State state;
    state.running = true;
    state.resized = true;
    state.touches = 0;

    mir_surface_set_event_handler(surface, on_event, &state);

    do
    {
        pthread_mutex_lock(&mutex);

        while (state.running && !state.resized && !state.touches)
            pthread_cond_wait(&change, &mutex);
        
        if (!state.running)
        {
            pthread_mutex_unlock(&mutex);
            break;
        }

        if (state.resized)
        {
            // mir_eglapp_swap_buffers updates the viewport for us...
            GLint viewport[4];
            glGetIntegerv(GL_VIEWPORT, viewport);
            int w = viewport[2], h = viewport[3];

            // TRANSPOSED projection matrix to convert from the Mir input
            // rectangle {{0,0},{w,h}} to GL screen rectangle {{-1,1},{2,2}}.
            GLfloat matrix[16] = {2.0f/w, 0.0f,   0.0f, 0.0f,
                                  0.0,   -2.0f/h, 0.0f, 0.0f,
                                  0.0f,   0.0f,   1.0f, 0.0f,
                                 -1.0f,   1.0f,   0.0f, 1.0f};
            // Note GL_FALSE: GLES does not support the transpose option
            glUniformMatrix4fv(projection, 1, GL_FALSE, matrix);
        }

        glClear(GL_COLOR_BUFFER_BIT);

        for (int t = 0; t < state.touches; ++t)
        {
            glUniform2f(translate, state.touch[t].x, state.touch[t].y);
            glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        }

        // Put the event loop back to sleep:
        state.resized = false;
        state.touches = 0;
        pthread_mutex_unlock(&mutex);

        mir_eglapp_swap_buffers();
    } while (mir_eglapp_running());

    mir_eglapp_shutdown();

    return 0;
}
