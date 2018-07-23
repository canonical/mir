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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir_toolkit/mir_client_library.h"
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <thread>
#include <memory>
#include <chrono>
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <signal.h>

#include "client_helpers.h"

namespace me = mir::examples;

namespace
{
class RenderProgram
{
public:
    RenderProgram(me::Context& context, unsigned int width, unsigned int height) :
        vertex(&vtex_shader_src, GL_VERTEX_SHADER),
        fragment(&frag_shader_src, GL_FRAGMENT_SHADER),
        program(vertex, fragment)
    {
        float square_side = 200.0f;
        float scale[2] {square_side/width, square_side/height};
        glUseProgram(program.program);
        vPositionAttr = glGetAttribLocation(program.program, "vPosition");
        glVertexAttribPointer(vPositionAttr, 4, GL_FLOAT, GL_FALSE, 0, vertex_data);
        posUniform = glGetUniformLocation(program.program, "pos");
        glClearColor(0.1, 0.1, 0.4, 1.0); //light blue
        glClear(GL_COLOR_BUFFER_BIT);
        auto scaleUniform = glGetUniformLocation(program.program, "scale");
        glUniform2fv(scaleUniform, 1, scale);
 
        context.swapbuffers();
        context.release_current();
    }

    void draw(float x, float y)
    {
        float pos[2] = {x, y}; 
        glUseProgram(program.program);
        glClear( GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT );
        glUniform2fv(posUniform, 1, pos);
        glEnableVertexAttribArray(vPositionAttr);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glDisableVertexAttribArray(vPositionAttr);
    }

    RenderProgram(RenderProgram const&) = delete;
    RenderProgram& operator=(RenderProgram const&) = delete; 
private:
    GLchar const*const frag_shader_src =
    {
        "precision mediump float;"
        "void main() {"
        "   gl_FragColor = vec4(0.0, 0.0, 0.0, 0.0);"
        "}"
    };
    GLchar const*const vtex_shader_src =
    {
        "attribute vec4 vPosition;"
        "uniform vec2 pos;"
        "uniform vec2 scale;"
        "void main() {"
        "   gl_Position = vec4(vPosition.xy * scale + pos, 0.0, 1.0);"
        "}"
    };
    me::Shader vertex;
    me::Shader fragment;
    me::Program program;
    GLfloat vertex_data[16]
    {
        -1.0, -1.0f, 0.0f, 1.0f,
        -1.0f,  1.0f, 0.0f, 1.0f,
         1.0f, -1.0f, 0.0f, 1.0f,
         1.0f,  1.0f, 0.0f, 1.0f,
    };
    GLuint vPositionAttr;
    GLuint posUniform; 
};

class SurfaceWithAHole
{
public:
    SurfaceWithAHole(me::Connection& connection) :
        context{connection, 1, width_and_height, width_and_height},
        window{connection, width_and_height, width_and_height, context},
        program{context, width_and_height, width_and_height}
    {
        context.make_current();
        program.draw(0.0, 0.0);
        context.swapbuffers();

        MirRectangle input_rectangles[] = {{0, 0, 500, 150}, {0, 0, 150, 500},
                                           {350, 0, 150, 500}, {0, 350, 500, 150}};
        auto spec = mir_create_window_spec(connection);
        mir_window_spec_set_input_shape(spec, input_rectangles, 4);
        mir_window_apply_spec(window, spec);
    }

    SurfaceWithAHole(SurfaceWithAHole const&) = delete;
    SurfaceWithAHole& operator=(SurfaceWithAHole const&) = delete;
private:
    me::Context context;
    me::NormalWindow window;
    RenderProgram program;

    static int const width_and_height = 500;
};
}

int main(int argc, char *argv[])
try
{
    using namespace std::literals::chrono_literals;

    auto arg = 0;
    char const* socket_file = nullptr;
    while ((arg = getopt (argc, argv, "m:")) != -1)
    {
        switch (arg)
        {
        case 'm':
            socket_file = optarg;
            break;
        default:
            throw std::invalid_argument("invalid command line argument");
        }
    }

    me::Connection connection(socket_file);
    SurfaceWithAHole surface(connection);

    sigset_t sigset;
    siginfo_t siginfo;
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGTERM);
    sigaddset(&sigset, SIGINT);
    sigprocmask(SIG_BLOCK, &sigset, nullptr);
    sigwaitinfo(&sigset, &siginfo);
    return 0;
}
catch(std::exception& e)
{
    std::cerr << "error : " << e.what() << std::endl;
    return 1;
}
