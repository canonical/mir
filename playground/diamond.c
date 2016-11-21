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
#include "mir_egl_platform_shim.h"
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

GLfloat const texcoords[] =
{
    0.0f, 0.0f,
    1.0f, 0.0f,
    0.0f, 1.0f,
    1.0f, 1.0f,
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

void render_diamond(Diamond* info, int width, int height)
{
    glClear(GL_COLOR_BUFFER_BIT);
    glViewport(0, 0, width, height);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, info->num_vertices);
}

Diamond setup_diamond_common()
{
    glClearColor(0.8f, 0.8f, 0.8f, 1.0f);
    char const vertex_shader_src[] =
        "attribute vec2 pos;                                \n"
        "attribute vec2 texcoord;                           \n"
        "varying vec2 v_texcoord;                           \n"
        "void main()                                        \n"
        "{                                                  \n"
        "    gl_Position = vec4(pos.x, pos.y, 0.0, 1.0);    \n"
        "    v_texcoord = texcoord;                         \n"
        "}                                                  \n";
    char const fragment_shader_src[] =
        "precision mediump float;                       \n"
        "varying vec2 v_texcoord;                       \n"
        "uniform sampler2D tex;                         \n"
        "void main()                                    \n"
        "{                                              \n"
        "   gl_FragColor = texture2D(tex, v_texcoord);  \n"
        "}                                              \n";

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
    info.texuniform = glGetUniformLocation(info.program, "tex");
    info.texcoord = glGetAttribLocation(info.program, "texcoord");
    info.num_vertices = num_vertices;

    glUniform1i(info.pos, 0);
    glUniform1i(info.texuniform, 0);
    glVertexAttribPointer(info.pos, 2, GL_FLOAT, GL_FALSE, 0, vertices);
    glVertexAttribPointer(info.texcoord, 2, GL_FLOAT, GL_FALSE, 0, texcoords);
    glEnableVertexAttribArray(info.pos);
    glEnableVertexAttribArray(info.texcoord);

    glActiveTexture(GL_TEXTURE0);
    glGenTextures(1, &info.texid);
    glBindTexture(GL_TEXTURE_2D, info.texid);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    return info;
}

Diamond setup_diamond_import(EGLImageKHR img, int use_shim)
{
    Diamond diamond = setup_diamond_common();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    if (use_shim)
    {
        future_driver_glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, img);
    }
    else
    {
        PFNGLEGLIMAGETARGETTEXTURE2DOESPROC glEGLImageTargetTexture2DOES =
            (PFNGLEGLIMAGETARGETTEXTURE2DOESPROC) eglGetProcAddress("glEGLImageTargetTexture2DOES");
        glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, img);
    }
    return diamond;
}

Diamond setup_diamond()
{
    Diamond diamond = setup_diamond_common();
    static unsigned char data[] = {
        0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0x00, 0x00, 0xFF,
        0xFF, 0x00, 0x00, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF
    };
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(
        GL_TEXTURE_2D, 0, GL_RGBA,
        2, 2, 0,
        GL_RGBA, GL_UNSIGNED_BYTE,
        data);
    return diamond;
}

void destroy_diamond(Diamond* info)
{
    glDeleteTextures(1, &info->texid);
    glDisableVertexAttribArray(info->pos);
    glDisableVertexAttribArray(info->texcoord);
    glDeleteShader(info->vertex_shader);
    glDeleteShader(info->fragment_shader);
    glDeleteProgram(info->program);
}
