/*
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
 * Author: Daniel van Vugt <daniel.van.vugt@canonical.com>
 */

#include "eglapp.h"
#include <assert.h>
#include <stdio.h>
#include <math.h>
#include <GLES2/gl2.h>
#include <mir_toolkit/mir_window.h>
#include <pthread.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

enum
{
    max_fingers = 10,
    max_samples_per_frame = 1000
};

typedef struct
{
    float x, y;
} Vec2;

typedef struct
{
    int samples;
    Vec2 sample[max_samples_per_frame];
} Finger;

typedef struct
{
    int fingers;
    Finger finger[max_fingers];
} TouchState;

typedef struct
{
    sigset_t sigs;
    pthread_mutex_t mutex;
    pthread_cond_t change_cv;
    bool changed;
    bool running;

    bool resized;
    TouchState touch;
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
    // Note the 0.5f to convert from pixel corner (GL) to middle (image)
    const float centrex = width/2 - 0.5f, centrey = height/2 - 0.5f;
    const Texel blank = {0, 0, 0, 0};
    const int radius = centrex - 1;
    const Texel ring[] =
    {
        {  0,   0,   0, 255},
        {  0,   0, 255, 255},
        {  0, 255,   0, 255},
        {255, 255,   0, 255},
        {255, 128,   0, 255},
        {255,   0,   0, 255},
    };
    const int rings = sizeof(ring) / sizeof(ring[0]);

    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            float dx = x - centrex, dy = y - centrey;
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

static void get_all_touch_points(const MirInputEvent *ievent, TouchState *touch)
{
    if (mir_input_event_get_type(ievent) == mir_input_event_type_pointer)
    {
        const MirPointerEvent *pevent =
            mir_input_event_get_pointer_event(ievent);
        if (mir_pointer_event_action(pevent) == mir_pointer_action_leave)
        {
            touch->fingers = 0;
        }
        else if (touch->finger[0].samples < max_samples_per_frame)
        {
            if (!touch->fingers)
                touch->finger[0].samples = 0;
            touch->fingers = 1;
            touch->finger[0].sample[touch->finger[0].samples++] = (Vec2)
            {
                mir_pointer_event_axis_value(pevent, mir_pointer_axis_x),
                mir_pointer_event_axis_value(pevent, mir_pointer_axis_y)
            };
        }
    }
    else if (mir_input_event_get_type(ievent) == mir_input_event_type_touch)
    {
        const MirTouchEvent *tevent = mir_input_event_get_touch_event(ievent);
        int n = mir_touch_event_point_count(tevent);
        if (n > max_fingers)
            n = max_fingers;
        for (int f = 0; f < n; ++f)
        {
            Finger *finger = touch->finger + f;
            if (f >= touch->fingers)
            {
                finger->samples = 0;
                touch->fingers = f + 1;
            }
            if (mir_touch_event_action(tevent, f) == mir_touch_action_up)
            {
                finger->samples = 0;
                continue;
            }
            if (finger->samples >= max_samples_per_frame)
                continue;
            finger->sample[finger->samples++] = (Vec2)
            {
                mir_touch_event_axis_value(tevent, f, mir_touch_axis_x),
                mir_touch_event_axis_value(tevent, f, mir_touch_axis_y)
            };
        }
    }
}

static void on_event(MirWindow *surface, const MirEvent *event, void *context)
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
        get_all_touch_points(mir_event_get_input_event(event), &state->touch);
        break;
    case mir_event_type_resize:
        egl_app_handle_resize_event(surface, mir_event_get_resize_event(event));
        state->resized = true;
        break;
    case mir_event_type_close_window:
        kill(getpid(), SIGTERM);
        break;
    default:
        break;
    }

    state->changed = true;
    pthread_cond_signal(&state->change_cv);
    pthread_mutex_unlock(&state->mutex);
}

static void* shutdown_handler(void* context)
{
    State *state = (State*)context;

    int signum;
    sigwait(&state->sigs, &signum);
    printf("Signal %d received. Good night.\n", signum);

    pthread_mutex_lock(&state->mutex);
    state->running = false;
    state->changed = true;
    pthread_cond_signal(&state->change_cv);
    pthread_mutex_unlock(&state->mutex);

    return NULL;
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
        "\n"
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
        "uniform float opacity;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 f = texture2D(texture, v_texcoord);\n"
        "    f.a *= opacity;\n"
        "    gl_FragColor = f;\n"
        "}\n";

    sigset_t sigs;
    sigemptyset(&sigs);
    sigaddset(&sigs, SIGINT);
    sigaddset(&sigs, SIGTERM);
    sigaddset(&sigs, SIGHUP);
    pthread_sigmask(SIG_BLOCK, &sigs, NULL);

    static unsigned int width = 0, height = 0;
    if (!mir_eglapp_init(argc, argv, &width, &height, NULL))
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
    glUniform1f(scale, 128.0f);

    GLint opacity = glGetUniformLocation(prog, "opacity");
    glUniform1f(opacity, 1.0f);

    GLint translate = glGetUniformLocation(prog, "translate");
    glUniform2f(translate, 0.0f, 0.0f);

    GLint projection = glGetUniformLocation(prog, "projection");

    GLuint tex = generate_target_texture();
    glBindTexture(GL_TEXTURE_2D, tex);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glViewport(0, 0, width, height);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE); // Behave like an accumulation buffer

    State state =
    {
        .mutex = PTHREAD_MUTEX_INITIALIZER,
        .change_cv = PTHREAD_COND_INITIALIZER,
        .changed = true,
        .running = true,
        .resized = true,
        .touch = {0, {{0, {{0.0f, 0.0f}}}}}
    };
    state.sigs = sigs;

    pthread_t shutdown_handler_thread;
    if (pthread_create(&shutdown_handler_thread, NULL, &shutdown_handler, &state))
    {
        printf("Failed creating shutdown handling thread\n");
        return 3;
    }

    MirWindow* window = mir_eglapp_native_window();
    mir_window_set_event_handler(window, on_event, &state);

    while (true)
    {
        pthread_mutex_lock(&state.mutex);

        while (state.running && !state.changed)
            pthread_cond_wait(&state.change_cv, &state.mutex);

        if (!state.running)
        {
            pthread_mutex_unlock(&state.mutex);
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
                                  0.0f,  -2.0f/h, 0.0f, 0.0f,
                                  0.0f,   0.0f,   1.0f, 0.0f,
                                 -1.0f,   1.0f,   0.0f, 1.0f};
            // Note GL_FALSE: GLES does not support the transpose option
            glUniformMatrix4fv(projection, 1, GL_FALSE, matrix);
            state.resized = false;
        }

        glClear(GL_COLOR_BUFFER_BIT);

        for (int f = 0; f < state.touch.fingers; ++f)
        {
            const Finger *finger = state.touch.finger + f;

            if (!finger->samples)
                continue;

            glUniform1f(opacity, 1.0f / finger->samples);

            for (int s = 0; s < finger->samples; ++s)
            {
                // Note the 0.5f to convert from pixel middle to corner (GL)
                glUniform2f(translate, finger->sample[s].x + 0.5f,
                                       finger->sample[s].y + 0.5f);
                glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
            }
        }

        // Just keep the latest sample for the zeroth finger (mouse pointer)
        if (state.touch.fingers)
        {
            state.touch.finger[0].sample[0] =
                state.touch.finger[0].sample[state.touch.finger[0].samples-1];
            state.touch.finger[0].samples = 1;
            state.touch.fingers = 1;
        }

        // Put the event loop back to sleep:
        state.changed = false;
        pthread_mutex_unlock(&state.mutex);

        mir_eglapp_swap_buffers();
    }

    mir_window_set_event_handler(window, NULL, NULL);
    mir_eglapp_cleanup();

    pthread_join(shutdown_handler_thread, NULL);
    return 0;
}
