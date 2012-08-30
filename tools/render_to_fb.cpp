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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir/graphics/platform.h"
#include "mir/graphics/display.h"

#include <GLES2/gl2.h>

#include "mir_image.h"

#define WIDTH 1280
#define HEIGHT 720

namespace mg=mir::graphics;

static const GLchar *vtex_shader_src =
{
    "attribute vec4 vPosition;\n"
    "attribute vec4 uvCoord;\n"
    "varying vec2 texcoord;\n"
    "uniform float slide;\n"
    "void main() {\n"
    "   gl_Position = vPosition;\n"
    "   texcoord = uvCoord.xy + vec2(slide);\n"
    "}\n"
};

static const GLchar *frag_shader_src =
{
    "precision mediump float;\n"
    "uniform sampler2D tex;\n"
    "varying vec2 texcoord;\n"
    "void main() {\n"
    "   gl_FragColor = texture2D(tex, texcoord);\n"
    "}\n"
};

const GLint num_vertex = 4;
GLfloat vertex_data[] =
{
    -1.0f, -1.0f, 0.0f, 1.0f,
    -1.0f,  1.0f, 0.0f, 1.0f,
    1.0f, -1.0f, 0.0f, 1.0f,
    1.0f,  1.0f, 0.0f, 1.0f,
};

GLfloat uv_data[] =
{
    1.0f, 1.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f

};

void setup_gl(GLuint& program, GLuint& vPositionAttr, GLuint& uvCoord, GLuint& slideUniform)
{
    glClearColor(0.0, 1.0, 0.0, 1.0);

    GLuint vtex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vtex_shader, 1, &vtex_shader_src, 0);
    glCompileShader(vtex_shader);

    GLuint frag_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(frag_shader, 1, &frag_shader_src, 0);
    glCompileShader(frag_shader);

    program = glCreateProgram();
    glAttachShader(program, vtex_shader);
    glAttachShader(program, frag_shader);
    glLinkProgram(program);

    vPositionAttr = glGetAttribLocation(program, "vPosition");
    glVertexAttribPointer(vPositionAttr, num_vertex, GL_FLOAT, GL_FALSE, 0, vertex_data);
    uvCoord = glGetAttribLocation(program, "uvCoord");
    glVertexAttribPointer(uvCoord, num_vertex, GL_FLOAT, GL_FALSE, 0, uv_data);

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(
        GL_TEXTURE_2D, 0, GL_RGBA,
        mir_image.width, mir_image.height, 0,
        GL_RGBA, GL_UNSIGNED_BYTE,
        mir_image.pixel_data);

    slideUniform = glGetUniformLocation(program, "slide");
}

void gl_render(GLuint program, GLuint vPosition, GLuint uvCoord,
               GLuint slideUniform, float slide)
{
    glUseProgram(program);

    glClear( GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT );

    glUniform1fv(slideUniform, 1, &slide);

    glActiveTexture(GL_TEXTURE0);
    glEnableVertexAttribArray(vPosition);
    glEnableVertexAttribArray(uvCoord);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, num_vertex);
    glDisableVertexAttribArray(uvCoord);
    glDisableVertexAttribArray(vPosition);
}

void adjust_vertices()
{
    float x_scale = (float) HEIGHT/ (1.5f * (float) mir_image.width);
    float y_scale = (float) WIDTH / (1.5f * (float) mir_image.height);
    for(int i=0; i< num_vertex; i++)
    {
        uv_data[i*4] *= x_scale;
        uv_data[i*4+1] *= y_scale;
    }
}

int main(int, char**)
{
    auto display = mg::create_display();

    GLuint program, vPosition, uvCoord, slideUniform;
    setup_gl(program, vPosition, uvCoord, slideUniform);
    adjust_vertices();

    float slide = 0.0f;
    for(;;)
    {
        gl_render(program, vPosition, uvCoord, slideUniform, slide);
        display->post_update();

        usleep(167);//60fps
        slide += 0.01f;
        if (slide > 1.0f)
            slide = 0.0f;
    }

    return 0;
}

