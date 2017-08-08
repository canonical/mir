/*
 * Copyright © 2016 Canonical Ltd.
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
 * Author: Brandon Schaefer <brandon.schaefer@canonical.com>
 */

#include "eglapp.h"
#include "mir_toolkit/mir_client_library.h"

#include <GLES2/gl2.h>
#include <assert.h>
#include <stdio.h>
#include <signal.h>
#include <math.h>

float mouse_x = 0.0f;
float mouse_y = 0.0f;
sig_atomic_t done = false;

static void shutdown(int signum)
{
    if (!done)
    {
        done = true;
        printf("Signal %d received. Good night.\n", signum);
    }
}

static float normalize(float xy, float wh, float size) { return (xy - wh/2.0f + size) / (wh/2.0f); }

static GLuint load_shader(const char* src, GLenum type)
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

static void handle_input_event(MirInputEvent const* event, MirWindow* surface)
{
    if (mir_input_event_get_type(event) == mir_input_event_type_pointer)
    {
        MirPointerEvent const* pev = mir_input_event_get_pointer_event(event);
        float dx = mir_pointer_event_axis_value(pev, mir_pointer_axis_relative_x);
        float dy = mir_pointer_event_axis_value(pev, mir_pointer_axis_relative_y);

        mouse_x += dx;
        // - because opengl coords
        mouse_y += -dy;
    }
    else if (mir_input_event_get_type(event) == mir_input_event_type_key)
    {
        MirKeyboardEvent const* kev = mir_input_event_get_keyboard_event(event);
        int key_code   = mir_keyboard_event_key_code(kev);

        if (mir_keyboard_event_action(kev) != mir_keyboard_action_up)
            return;

        if (key_code == XKB_KEY_q)
        {
            done = true;
        }
        else if (key_code == XKB_KEY_p)
        {
            // We start out grabbed
            static bool grabbed = true;

            MirWindowSpec* spec = mir_create_window_spec(mir_eglapp_native_connection());
            if (!grabbed)
            {
                mir_window_spec_set_pointer_confinement(spec, mir_pointer_confined_to_window);
                mir_window_spec_set_name(spec, "");
            }
            else
            {
                mir_window_spec_set_pointer_confinement(spec, mir_pointer_unconfined);
                mir_window_spec_set_name(spec, mir_default_cursor_name);
            }

            grabbed = !grabbed;

            mir_window_apply_spec(surface, spec);
            mir_window_spec_release(spec);
        }
        else if (key_code == XKB_KEY_f)
        {
            static bool fullscreen = false;
            MirWindowSpec* spec = mir_create_window_spec(mir_eglapp_native_connection());
            MirWindowState state = mir_window_state_restored;

            if (!fullscreen)
            {
                state = mir_window_state_fullscreen;
            }

            fullscreen = !fullscreen;

            mir_window_spec_set_state(spec, state);
            mir_window_apply_spec(surface, spec);
            mir_window_spec_release(spec);
        }
    }
}

static void handle_event(MirWindow* surface, MirEvent const* event, void* context)
{
    (void) context;
    switch (mir_event_get_type(event))
    {
    case mir_event_type_input:
        handle_input_event(mir_event_get_input_event(event), surface);
        break;
    default:
        break;
    }
}

static float fmodulus(float x, float y)
{
    float mod = fmod(x, y);
    if (mod < 0.0f)
    {
        mod += y;
    }

    return mod;
}

int main(int argc, char* argv[])
{
    char const v_shader_src[] =
    "attribute vec4 v_position;  \n"
    "                            \n"
    "void main()                 \n"
    "{                           \n"
    "  gl_Position = v_position; \n"
    "}                           \n";

    char const f_shader_src[] =
    "#ifdef GL_ES              \n"
    "precision mediump float;  \n"
    "#endif                    \n"
    "                          \n"
    "void main()               \n"
    "{                         \n"
    "  gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0); \n"
    "}                         \n";

    int sqaure_size = 10;
    unsigned int width = 0, height = 0;
    if (!mir_eglapp_init(argc, argv, &width, &height, NULL))
        return 1;

    width  /= 2;
    height /= 2;

    MirWindowSpec* spec = mir_create_window_spec(mir_eglapp_native_connection());
    mir_window_spec_set_width (spec, width);
    mir_window_spec_set_height(spec, height);

    mir_window_apply_spec(mir_eglapp_native_window(), spec);
    mir_window_spec_release(spec);

    mouse_x = width  / 2.0;
    mouse_y = height / 2.0;

    GLuint vshader = load_shader(v_shader_src, GL_VERTEX_SHADER);
    assert(vshader);
    GLuint fshader = load_shader(f_shader_src, GL_FRAGMENT_SHADER);
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

    MirWindow* window = mir_eglapp_native_window();
    mir_window_set_event_handler(window, handle_event, NULL);

    spec = mir_create_window_spec(mir_eglapp_native_connection());
    mir_window_spec_set_pointer_confinement(spec, mir_pointer_confined_to_window);

    // Hide cursor
    mir_window_spec_set_cursor_name(spec, mir_disabled_cursor_name);
    mir_window_apply_spec(window, spec);
    mir_window_spec_release(spec);

    glClearColor(0.3f, 0.3f, 0.3f, 1.0f);

    signal(SIGINT, shutdown);
    signal(SIGTERM, shutdown);
    signal(SIGHUP, shutdown);

    while (!done)
    {
        glClear(GL_COLOR_BUFFER_BIT);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        /*|-----------|
         *|  3    4-0 |
         *|     /     |
         *| 5-1    2  |
         *|-----------|
        **/
        float mod_x = fmodulus(mouse_x, width);
        float mod_y = fmodulus(mouse_y, height);

        float four_zero_x = normalize(mod_x, width,  sqaure_size);
        float four_zero_y = normalize(mod_y, height, sqaure_size);

        float five_one_x  = normalize(mod_x, width, 0.0f);
        float five_one_y  = normalize(mod_y, height, 0.0f);

        float two_x       = normalize(mod_x, width, sqaure_size);
        float two_y       = normalize(mod_y, height, 0.0f);

        float three_x     = normalize(mod_x, width, 0.0f);
        float three_y     = normalize(mod_y, height, sqaure_size);

        GLfloat square_vert[] = { five_one_x , five_one_y , 0.0f,
                                  two_x      , two_y      , 0.0f,
                                  three_x    , three_y    , 0.0f,
                                  four_zero_x, four_zero_y, 0.0f };

        glEnableVertexAttribArray(0);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, square_vert);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        mir_eglapp_swap_buffers();
    }

    mir_eglapp_cleanup();
    return 0;
}
