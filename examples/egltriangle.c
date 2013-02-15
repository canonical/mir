/*
 * Copyright © 2013 Canonical Ltd.
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

#include "./eglapp.h"
#include <assert.h>
#include <stdio.h>
#include <unistd.h> /* sleep() */
#include <GLES2/gl2.h>

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
        "uniform vec4 col;                               \n"
        "void main()                                     \n"
        "{                                               \n"
        "    gl_FragColor = col;                         \n"
        "}                                               \n";

    const GLfloat vertices[] =
    {
        0.0f, 1.0f,
       -1.0f,-0.866f,
        1.0f,-0.866f,
    };
    GLuint vshader, fshader, prog;
    GLint linked, col, vpos;
    int width = 512, height = 512;

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

    vpos = glGetAttribLocation(prog, "vPosition");
    col = glGetUniformLocation(prog, "col");
    glUniform4f(col, ORANGE, 1.0f);

    glVertexAttribPointer(vpos, 2, GL_FLOAT, GL_FALSE, 0, vertices);
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
