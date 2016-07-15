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
 * Author: Brandon Schaefer <brandon.schaefer@canonical.com>
 */

#include "eglapp.h"
#include <cassert>
#include <GLES2/gl2.h>
#include "mir_toolkit/mir_client_library.h"
#include <stdio.h>
#include <atomic>

std::atomic<int> mouse_x{0};
std::atomic<int> mouse_y{0};
std::atomic<bool> done{false};

constexpr float m_to_v(float xy, float wh, float size = 0.0f) { return (xy - wh/2.0f + size) / (wh/2.0f); }

static GLuint load_shader(const char* src, GLenum type)
{
    GLuint shader = glCreateShader(type);
    if (shader)
    {
        GLint compiled;
        glShaderSource(shader, 1, &src, nullptr);
        glCompileShader(shader);
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
        if (!compiled)
        {
            GLchar log[1024];
            glGetShaderInfoLog(shader, sizeof log - 1, nullptr, log);
            log[sizeof log - 1] = '\0';
            printf("load_shader compile failed: %s\n", log);
            glDeleteShader(shader);
            shader = 0;
        }
    }
    return shader;
}

static void handle_input_event(MirInputEvent const* event)
{
    if (mir_input_event_get_type(event) == mir_input_event_type_pointer)
    {
        MirPointerEvent const* pev = mir_input_event_get_pointer_event(event);
        auto dx = mir_pointer_event_axis_value(pev, mir_pointer_axis_relative_x);
        auto dy = mir_pointer_event_axis_value(pev, mir_pointer_axis_relative_y);

        mouse_x.fetch_add(dx);
        // - because opengl coords
        mouse_y.fetch_add(-dy);
    }
    else if(mir_input_event_get_type(event) == mir_input_event_type_key)
    {
        MirKeyboardEvent const* kev = mir_input_event_get_keyboard_event(event);
        if (mir_keyboard_event_key_code(kev) == XKB_KEY_q)
        {
            done = true;
        }
    }
}

static void handle_event(MirSurface* surface, MirEvent const* event, void* context)
{
    (void) surface;
    (void) context;
    switch (mir_event_get_type(event))
    {
    case mir_event_type_input:
        handle_input_event(mir_event_get_input_event(event));
        break;
    default:
        break;
    }
}

int main(int argc, char* argv[])
{
    char const v_shader_src[] =
    "attribute vec4 v_position;  \n"
    "attribute vec4 a_color;     \n"
    "varying vec4 v_color;       \n"
    "                            \n"
    "void main()                 \n"
    "{                           \n"
    "  v_color     = a_color;    \n"
    "  gl_Position = v_position; \n"
    "}                           \n";

    char const f_shader_src[] =
    "#ifdef GL_ES              \n"
    "precision mediump float;  \n"
    "#endif                    \n"
    "                          \n"
    "varying vec4 v_color;     \n"
    "                          \n"
    "void main()               \n"
    "{                         \n"
    "  gl_FragColor = v_color; \n"
    "}                         \n";

    int sqaure_size = 10;
    unsigned int width = 640, height = 400;
    if (!mir_eglapp_init(argc, argv, &width, &height))
        return 1;

    mouse_x = width  / 2;
    mouse_y = height / 2;

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
        glGetProgramInfoLog(prog, sizeof log - 1, nullptr, log);
        log[sizeof log - 1] = '\0';
        printf("Link failed: %s\n", log);
        return 2;
    }

    glUseProgram(prog);

    MirSurface* surface = mir_eglapp_native_surface();
    mir_surface_set_event_handler(surface, handle_event, nullptr);

    MirSurfaceSpec* spec = mir_connection_create_spec_for_changes(mir_eglapp_native_connection());
    mir_surface_spec_set_pointer_confinement(spec, mir_pointer_confined_to_surface);

    mir_surface_apply_spec(surface, spec);
    mir_surface_spec_release(spec);

    // Hide cursor
    mir_surface_configure_cursor(surface, NULL);

    glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
    GLfloat square_color[] = {1.0f, 0.0f, 0.0f, 1.0f};

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
        float four_zero_x = m_to_v(mouse_x % width, width,  sqaure_size);
        float four_zero_y = m_to_v(mouse_y % height, height, sqaure_size);

        float five_one_x  = m_to_v(mouse_x % width, width);
        float five_one_y  = m_to_v(mouse_y % height, height);

        float two_x       = m_to_v(mouse_x % width, width, sqaure_size);
        float two_y       = m_to_v(mouse_y % height, height);

        float three_x     = m_to_v(mouse_x % width, width);
        float three_y     = m_to_v(mouse_y % height, height, sqaure_size);

        GLfloat square_vert[] = { five_one_x , five_one_y , 0.0f,
                                  two_x      , two_y      , 0.0f,
                                  three_x    , three_y    , 0.0f,
                                  four_zero_x, four_zero_y, 0.0f };

        glEnableVertexAttribArray(0);

        glVertexAttrib4fv(1, square_color);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, square_vert);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        mir_eglapp_swap_buffers();
    }

    mir_eglapp_shutdown();
    return 0;
}
