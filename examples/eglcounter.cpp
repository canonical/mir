/*
 * Copyright © 2014 Canonical Ltd.
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
 *         Alberto Aguirre <alberto.aguirre@canonical.com>
 */

#include "eglapp.h"
#include <assert.h>
#include <stdio.h>
#include <GLES2/gl2.h>

#include <vector>
#include <stdexcept>
#include <iostream>
#include <algorithm>

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

class DrawableDigit
{
public:
    DrawableDigit(std::initializer_list<std::vector<GLubyte>> list)
    {
        for (auto const& arg : list)
        {
            indices.insert(indices.end(), arg.begin(), arg.end());
        }
    }

    void draw()
    {
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_BYTE, indices.data());
    }

private:
    DrawableDigit() = delete;
    std::vector<GLubyte> indices;
};

class TwoDigitCounter
{
public:
    TwoDigitCounter(GLuint prog, float thickness, unsigned int max_count)
        : count{0}, max_count{std::min(max_count, 99u)}, program{prog},
          vertices{create_vertices(thickness)}, digit_drawables{create_digits()}
    {
        vpos = glGetAttribLocation(program, "vPosition");
        if (vpos == -1)
            throw std::runtime_error("Failed to obtain vPosition attribute");

        xoff = glGetUniformLocation(program, "xOffset");
        if (xoff == -1)
            throw std::runtime_error("Failed to obtain xoff uniform");

        xscale = glGetUniformLocation(program, "xScale");
        if (xscale == -1)
            throw std::runtime_error("Failed to obtain xscale uniform");

        glUniform1f(xscale, 0.4f);
        glVertexAttribPointer(vpos, 2, GL_FLOAT, GL_FALSE, 0, vertices.data());
        glEnableVertexAttribArray(0);
    }

    void draw()
    {
        drawLeftDigit();
        drawRightDigit();
        nextCount();
    }

private:
    void draw(int digit)
    {
        if (static_cast<size_t>(digit) > digit_drawables.size())
            throw std::logic_error("invalid digit to draw");

        digit_drawables[digit].draw();
    }

    void drawRightDigit()
    {
        glUniform1f(xoff, 0.5f);
        draw(count % 10);
    }

    void drawLeftDigit()
    {
        glUniform1f(xoff, -0.5f);
        draw(count/10);
    }

    void nextCount()
    {
        count++;
        if (count > max_count) count = 0;
    }

    static std::vector<GLfloat> create_vertices(GLfloat t)
    {
        return std::vector<GLfloat>{
            -1.0f,   -1.0f,
            -1.0f+t, -1.0f,
             1.0f-t, -1.0f,

             1.0f,   -1.0f,
            -1.0f,   -1.0f+t,
             1.0f,   -1.0f+t,

            -1.0f,   -t/2.0f,
            -1.0f+t, -t/2.0f,
             1.0f-t, -t/2.0f,

             1.0f,   -t/2.0f,
            -1.0f,    t/2.0f,
             1.0f,    t/2.0f,

            -1.0f,    1.0f-t,
             1.0f,    1.0f-t,
            -1.0f,    1.0f,

            -1.0f+t,  1.0f,
             1.0f-t,  1.0f,
             1.0f,    1.0f,
        };
    }

    static std::vector<DrawableDigit> create_digits()
    {
        std::vector<DrawableDigit> digits;
        /*
         * Vertex indices for quads which can be used to make
         * simple number "font" shapes
         */
        std::vector<GLubyte> top{12, 14, 13, 13, 14, 17};
        std::vector<GLubyte> bottom{0, 4, 3, 3, 4, 5};
        std::vector<GLubyte> middle{6, 10, 9, 9, 10, 11};
        std::vector<GLubyte> left{0, 14, 1, 1, 14, 15};
        std::vector<GLubyte> right{2, 16, 3, 3, 16, 17};

        std::vector<GLubyte> top_left{6, 14, 7, 7, 14, 15};
        std::vector<GLubyte> top_right{8, 16, 9, 9, 16, 17};
        std::vector<GLubyte> bottom_left{0, 6, 1, 1, 6, 7};
        std::vector<GLubyte> bottom_right{2, 8, 3, 3, 8, 9};

        digits.push_back({top, bottom, left, right});
        digits.push_back({right});
        digits.push_back({top, middle, bottom, top_right, bottom_left});
        digits.push_back({top, middle, bottom, right});
        digits.push_back({top_left, middle, right});
        digits.push_back({top, middle, bottom, top_left, bottom_right});
        digits.push_back({top, middle, bottom, left, bottom_right});
        digits.push_back({top, right});
        digits.push_back({top, middle, bottom, left, right});
        digits.push_back({top, middle, right, top_left});
        return digits;
    }

    unsigned int count;
    unsigned int const max_count;

    GLint vpos;
    GLint xoff;
    GLint xscale;
    GLuint program;

    std::vector<GLfloat> const vertices;
    std::vector<DrawableDigit> digit_drawables;
};

/* Colours from http://design.ubuntu.com/brand/colour-palette */
#define MID_AUBERGINE(x) (x)*0.368627451f, (x)*0.152941176f, (x)*0.31372549f
#define ORANGE        0.866666667f, 0.282352941f, 0.141414141f

int main(int argc, char *argv[])
{
    const char vshadersrc[] =
        "attribute vec4 vPosition;            \n"
        "uniform float xOffset;               \n"
        "uniform float xScale;                \n"
        "void main()                          \n"
        "{                                    \n"
        "    vec4 vertex;                     \n"
        "    vertex = vPosition;              \n"
        "    vertex.x *= xScale;              \n"
        "    vertex.x += xOffset;             \n"
        "    gl_Position = vertex;            \n"
        "}                                    \n";

    const char fshadersrc[] =
        "precision mediump float;             \n"
        "uniform vec4 col;                    \n"
        "void main()                          \n"
        "{                                    \n"
        "    gl_FragColor = col;              \n"
        "}                                    \n";

    GLuint vshader, fshader, prog;
    GLint linked, col;
    unsigned int width = 512, height = 256;

    if (!mir_eglapp_init(argc, argv, &width, &height, NULL))
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

    float const opacity = mir_eglapp_background_opacity;
    glClearColor(MID_AUBERGINE(opacity), opacity);
    glViewport(0, 0, width, height);

    glUseProgram(prog);

    col = glGetUniformLocation(prog, "col");
    glUniform4f(col, ORANGE, 1.0f);

    float thickness = 0.2f;
    unsigned int max_counter = 60;
    TwoDigitCounter counter(prog, thickness, max_counter);

    while (mir_eglapp_running())
    {
        glClear(GL_COLOR_BUFFER_BIT);
        counter.draw();
        mir_eglapp_swap_buffers();
    }

    mir_eglapp_cleanup();

    return 0;
}
