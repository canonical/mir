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
#include <condition_variable>
#include <atomic>

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
        float square_side = 400.0f;
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
        "   gl_FragColor = vec4(0.0, 0.0, 1.0, 1.0);"
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

class SquareRenderingSurface
{
public:
    SquareRenderingSurface(me::Connection& connection, int swap_interval) :
        dimensions(active_output_dimensions(connection)),
        context{connection, swap_interval, dimensions.width, dimensions.height},
        window{connection, dimensions.width, dimensions.height, context},
        program{context, dimensions.width, dimensions.height},
        pos{Pos{0,0}},
        worker{[this] { do_work(); }}
    {
        mir_window_set_event_handler(window, &on_event, this);
    }

    ~SquareRenderingSurface()
    {
        {
            std::lock_guard<decltype(mutex)> lock{mutex};
            running = false;
            cv.notify_one();
        }

        if (worker.joinable()) worker.join();
    }

    void on_event(MirEvent const* event)
    {
        switch (mir_event_get_type(event))
        {
        case mir_event_type_resize:
        {
            MirResizeEvent const* resize = mir_event_get_resize_event(event);
            int const new_width = mir_resize_event_get_width(resize);
            int const new_height = mir_resize_event_get_height(resize);

            mir_render_surface_set_size(context.mir_surface(), new_width, new_height);
            MirWindowSpec* spec = mir_create_window_spec(mir_window_get_connection(window));
            mir_window_spec_add_render_surface(spec, context.mir_surface(), new_width, new_height, 0, 0);
            mir_window_apply_spec(window, spec);
            mir_window_spec_release(spec);
            break;
        }

        case mir_event_type_input:
        {
            float x{0.0f};
            float y{0.0f};
            auto ievent = mir_event_get_input_event(event);
            if (mir_input_event_get_type(ievent) == mir_input_event_type_touch)
            {
                auto tev = mir_input_event_get_touch_event(ievent);
                x = mir_touch_event_axis_value(tev, 0, mir_touch_axis_x);
                y = mir_touch_event_axis_value(tev, 0, mir_touch_axis_y);
            }
            else if (mir_input_event_get_type(ievent) == mir_input_event_type_pointer)
            {
                auto pev = mir_input_event_get_pointer_event(ievent);
                x = mir_pointer_event_axis_value(pev, mir_pointer_axis_x);
                y = mir_pointer_event_axis_value(pev, mir_pointer_axis_y);
            }
            else
            {
                return;
            }

            pos = Pos{x, y};
            cv.notify_one();
            break;
        }

        case mir_event_type_close_window:
            kill(getpid(), SIGTERM);
            break;

        default:;
        }
    }

    SquareRenderingSurface(SquareRenderingSurface const&) = delete;
    SquareRenderingSurface& operator=(SquareRenderingSurface const&) = delete;
private:
    struct OutputDimensions
    {
        unsigned int const width;
        unsigned int const height;
    } const dimensions;

    me::Context context;
    me::NormalWindow window;
    RenderProgram program;

    OutputDimensions active_output_dimensions(MirConnection* connection)
    {
        unsigned int width{0};
        unsigned int height{0};
        auto display_config = mir_connection_create_display_configuration(connection);
        auto num_outputs = mir_display_config_get_num_outputs(display_config);
        for (auto i = 0; i < num_outputs; i++)
        {
            auto output = mir_display_config_get_output(display_config, i);
            auto state = mir_output_get_connection_state(output);
            if (state == mir_output_connection_state_connected && mir_output_is_enabled(output))
            {
                auto mode = mir_output_get_current_mode(output);
                width  = mir_output_mode_get_width(mode);
                height = mir_output_mode_get_height(mode);
                break;
            }
        }
        mir_display_config_release(display_config);
        if (width == 0 || height == 0)
            throw std::logic_error("could not determine display size");
        return {width, height};
    }

    static void on_event(MirWindow*, const MirEvent *event, void *context)
    {
        auto surface = reinterpret_cast<SquareRenderingSurface*>(context);
        if (surface) surface->on_event(event);
    }

private:
    void do_work()
    {
        std::unique_lock<decltype(mutex)> lock(mutex);

        while (true)
        {
            cv.wait(lock);

            if (!running) return;

            Pos  pos = this->pos;

            context.make_current();
            program.draw(
                pos.x/static_cast<float>(dimensions.width)*2.0 - 1.0,
                pos.y/static_cast<float>(dimensions.height)*-2.0 + 1.0);
            context.swapbuffers();
        }
    }

    struct Pos { float x; float y; };
    std::atomic<Pos> pos;

    std::thread worker;
    std::condition_variable cv;
    std::mutex mutex;
    bool running{true};
};
}

int main(int argc, char *argv[])
try
{
    using namespace std::literals::chrono_literals;

    auto arg = 0;
    char const* socket_file = nullptr;
    int swap_interval = 1;
    while ((arg = getopt (argc, argv, "nm:")) != -1)
    {
        switch (arg)
        {
        case 'm':
            socket_file = optarg;
            break;
        case 'n':
            swap_interval = 0;
            break;
        default:
            throw std::invalid_argument("invalid command line argument");
        }
    }

    me::Connection connection(socket_file);
    SquareRenderingSurface surface(connection, swap_interval);

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
