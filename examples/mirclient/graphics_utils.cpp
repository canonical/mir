/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
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

#include "graphics.h"
#include "mir_image.h"

#include <cmath>
#include <chrono>

namespace md=mir::draw;

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
    "#ifdef GL_ES\n"
    "precision mediump float;\n"
    "#endif\n"
    "uniform sampler2D tex;\n"
    "varying vec2 texcoord;\n"
    "void main() {\n"
    "   gl_FragColor = texture2D(tex, texcoord);\n"
    "}\n"
};

const GLint num_vertex = 4;
GLfloat vertex_data[] =
{
    -0.5f, -0.5f, 0.0f, 1.0f,
    -0.5f,  0.5f, 0.0f, 1.0f,
     0.5f, -0.5f, 0.0f, 1.0f,
     0.5f,  0.5f, 0.0f, 1.0f,
};

GLfloat uv_data[] =
{
    1.0f, 1.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f

};

md::glAnimationBasic::glAnimationBasic()
 : program(-1),
   vPositionAttr(-1),
   uvCoord(-1),
   slideUniform(-1),
   slide(0.0)
{
}
int md::glAnimationBasic::texture_width()
{
    return mir_image.width;
}

int md::glAnimationBasic::texture_height()
{
    return mir_image.height;
}

void md::glAnimationBasic::init_gl()
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
    glVertexAttribPointer(vPositionAttr, 4, GL_FLOAT, GL_FALSE, 0, vertex_data);
    uvCoord = glGetAttribLocation(program, "uvCoord");
    glVertexAttribPointer(uvCoord, 4, GL_FLOAT, GL_FALSE, 0, uv_data);

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

void md::glAnimationBasic::render_gl()
{
    glUseProgram(program);

    glClear( GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT );

    glUniform1fv(slideUniform, 1, &slide);

    glActiveTexture(GL_TEXTURE0);
    glEnableVertexAttribArray(vPositionAttr);
    glEnableVertexAttribArray(uvCoord);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, num_vertex);
    glDisableVertexAttribArray(uvCoord);
    glDisableVertexAttribArray(vPositionAttr);
}

void md::glAnimationBasic::step()
{
    typedef std::chrono::high_resolution_clock hr_clock;
    typedef std::chrono::duration<double> seconds_double;

    auto elapsed = hr_clock::now().time_since_epoch();
    auto elapsed_seconds = std::chrono::duration_cast<seconds_double>(elapsed).count();

    double i;
    /* slide increases 0.01 per 1/60s */
    slide = modf(0.6 * elapsed_seconds, &i);
}
