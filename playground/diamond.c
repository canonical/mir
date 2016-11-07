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

#include "diamond.h"
#include <assert.h>
#include <stdio.h>
#include <unistd.h>

int const num_vertices = 4; 
GLfloat const vertices[] =
{
    1.0f, 0.0f,
    0.0f, 1.0f,
    0.0f, -1.0f,
    -1.0f, 0.0f,
};

GLfloat const colors[] =
{
    1.0f, 0.2f, 0.2f, 1.0f,
    0.2f, 1.0f, 0.2f, 1.0f,
    0.2f, 0.2f, 1.0f, 1.0f,
    0.2f, 0.2f, 0.2f, 1.0f,
};

static GLuint load_shader(const char *src, GLenum type)
{
    GLuint shader = glCreateShader(type);
    assert(shader);

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
        assert(-1);
    }
    return shader;
}

void render_diamond(Diamond* info, EGLDisplay egldisplay, EGLSurface eglsurface)
{
    EGLint width = -1;
    EGLint height = -1;
    glClear(GL_COLOR_BUFFER_BIT);
    if (eglQuerySurface(egldisplay, eglsurface, EGL_WIDTH, &width) &&
        eglQuerySurface(egldisplay, eglsurface, EGL_HEIGHT, &height))
    {
        glViewport(0, 0, width, height);
    }
    glDrawArrays(GL_TRIANGLE_STRIP, 0, info->num_vertices);
}

Diamond setup_diamond()
{
    char const vertex_shader_src[] =
        "attribute vec2 pos;                                \n"
        "attribute vec4 color;                              \n"
        "varying   vec4 dest_color;                         \n"
        "void main()                                        \n"
        "{                                                  \n"
        "    dest_color = color;                            \n"
        "    gl_Position = vec4(pos.x, pos.y, 0.0, 1.0);    \n"
        "}                                                  \n";
    char const fragment_shader_src[] =
        "precision mediump float;             \n"
        "varying   vec4 dest_color;           \n"
        "void main()                          \n"
        "{                                    \n"
        "    gl_FragColor = dest_color;       \n"
        "}                                    \n";

    GLint linked = 0;
    Diamond info;

    info.vertex_shader = load_shader(vertex_shader_src, GL_VERTEX_SHADER);
    info.fragment_shader = load_shader(fragment_shader_src, GL_FRAGMENT_SHADER);
    info.program = glCreateProgram();
    assert(info.program);

    glAttachShader(info.program, info.vertex_shader);
    glAttachShader(info.program, info.fragment_shader);
    glLinkProgram(info.program);
    glGetProgramiv(info.program, GL_LINK_STATUS, &linked);
    if (!linked)
    {
        GLchar log[1024];
        glGetProgramInfoLog(info.program, sizeof log - 1, NULL, log);
        log[sizeof log - 1] = '\0';
        printf("Link failed: %s\n", log);
        assert(-1);
    }

    glUseProgram(info.program);
    info.pos = glGetAttribLocation(info.program, "pos");
    info.color = glGetAttribLocation(info.program, "color");
    info.num_vertices = num_vertices;
    glVertexAttribPointer(info.pos, 2, GL_FLOAT, GL_FALSE, 0, vertices);
    glVertexAttribPointer(info.color, 4, GL_FLOAT, GL_FALSE, 0, colors);
    glEnableVertexAttribArray(info.pos);
    glEnableVertexAttribArray(info.color);
    return info;
}

void destroy_diamond(Diamond* info)
{
    glDisableVertexAttribArray(info->pos);
    glDisableVertexAttribArray(info->color);
    glDeleteShader(info->vertex_shader);
    glDeleteShader(info->fragment_shader);
    glDeleteProgram(info->program);
}
