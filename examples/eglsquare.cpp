/*
 * Copyright © 2015 Canonical Ltd.
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

namespace
{
class Connection
{
public:
    Connection(char const* socket_file) :
        connection(mir_connect_sync(socket_file, __PRETTY_FUNCTION__))
    {
        if (!mir_connection_is_valid(connection))
            throw std::runtime_error("could not connect to server");
    }
    ~Connection()
    {
        mir_connection_release(connection);
    }
    operator MirConnection*() { return connection; }
    Connection(Connection const&) = delete;
    Connection& operator=(Connection const&) = delete;
private:
    MirConnection* connection;
};

class Context
{
public:
    Context(Connection& connection, MirSurface* surface, int swap_interval) :
        native_display(reinterpret_cast<EGLNativeDisplayType>(
            mir_connection_get_egl_native_display(connection))),
        native_window(reinterpret_cast<EGLNativeWindowType>(
            mir_buffer_stream_get_egl_native_window(mir_surface_get_buffer_stream(surface)))),
        display(native_display),
        config(chooseconfig(display.disp)),
        surface(display.disp, config, native_window),
        context(display.disp, config)
    {
        make_current();
        eglSwapInterval(display.disp, swap_interval);
    }
    void make_current()
    {
        if (eglMakeCurrent(display.disp, surface.surface, surface.surface, context.context) == EGL_FALSE)
            throw(std::runtime_error("could not makecurrent"));
    }
    void release_current()
    {
        if (eglMakeCurrent(display.disp, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT) == EGL_FALSE)
            throw(std::runtime_error("could not makecurrent"));
    }
    void swapbuffers()
    {
        if (eglSwapBuffers(display.disp, surface.surface) == EGL_FALSE)
            throw(std::runtime_error("could not swapbuffers"));
    }
    Context(Context const&) = delete;
    Context& operator=(Context const&) = delete;
private:
    EGLConfig chooseconfig(EGLDisplay disp)
    {
        int n{0};
        EGLConfig egl_config;
        EGLint attribs[] = {
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_RED_SIZE, 8,
            EGL_GREEN_SIZE, 8,
            EGL_BLUE_SIZE, 8,
            EGL_ALPHA_SIZE, 8,
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
            EGL_NONE };
        if (eglChooseConfig(disp, attribs, &egl_config, 1, &n) != EGL_TRUE || n != 1)
            throw std::runtime_error("could not find egl config");
        return egl_config;
    }
    EGLNativeDisplayType native_display;
    EGLNativeWindowType native_window;
    struct Display
    {
        Display(EGLNativeDisplayType native) :
            disp(eglGetDisplay(native))
        {
            int major{0}, minor{0};
            if (disp == EGL_NO_DISPLAY)
                throw std::runtime_error("no egl display");
            if (eglInitialize(disp, &major, &minor) != EGL_TRUE || major != 1 || minor != 4)
                throw std::runtime_error("could not init egl");
        }
        ~Display()
        {
            eglTerminate(disp);
        }
        EGLDisplay disp;
    } display;
    EGLConfig config;
    struct Surface
    {
        Surface(EGLDisplay display, EGLConfig config, EGLNativeWindowType native_window) :
            disp(display),
            surface(eglCreateWindowSurface(disp, config, native_window, NULL))
        {
            if (surface == EGL_NO_SURFACE)
                throw std::runtime_error("could not create egl surface");
        }
        ~Surface() {
            if (eglGetCurrentSurface(EGL_DRAW) == surface)
                eglMakeCurrent(disp, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
            eglDestroySurface(disp, surface);
         }
        EGLDisplay disp;
        EGLSurface surface;
    } surface;
    struct EglContext
    {
        EglContext(EGLDisplay disp, EGLConfig config) :
            disp(disp),
            context(eglCreateContext(disp, config, EGL_NO_CONTEXT, context_attribs))
        {
            if (context == EGL_NO_CONTEXT)
                throw std::runtime_error("could not create egl context");
        }
        ~EglContext()
        {
            eglDestroyContext(disp, context);
        }
        EGLint context_attribs[3] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
        EGLDisplay disp;
        EGLContext context;
    } context;
};

class RenderProgram
{
public:
    RenderProgram(Context& context, unsigned int width, unsigned int height) :
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
    struct Shader
    {
        Shader(GLchar const* const* src, GLuint type) :
            shader(glCreateShader(type))
        {
            glShaderSource(shader, 1, src, 0);
            glCompileShader(shader);
        }
        ~Shader()
        {
            glDeleteShader(shader);
        }
        GLuint shader;
    } vertex, fragment;
    struct Program
    {
        Program(Shader& vertex, Shader& fragment) :
            program(glCreateProgram())
        {
            glAttachShader(program, vertex.shader);
            glAttachShader(program, fragment.shader);
            glLinkProgram(program);
        }
        ~Program()
        {
            glDeleteProgram(program);
        }
        GLuint program;
    } program;
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

class Surface
{
public:
    Surface(Connection& connection, int swap_interval) :
        dimensions(active_output_dimensions(connection)),
        surface{create_surface(connection, dimensions), surface_deleter},
        context{connection, surface.get(), swap_interval},
        program{context, dimensions.width, dimensions.height}
    {
    }

    void on_event(MirEvent const* ev)
    {
        if (mir_event_get_type(ev) != mir_event_type_input)
            return;
        float x{0.0f};
        float y{0.0f};
        auto ievent = mir_event_get_input_event(ev);
        if (mir_input_event_get_type(ievent) == mir_input_event_type_touch)
        {
            auto tev = mir_input_event_get_touch_input_event(ievent);
            x = mir_touch_input_event_get_touch_axis_value(tev, 0, mir_touch_input_axis_x);
            y = mir_touch_input_event_get_touch_axis_value(tev, 0, mir_touch_input_axis_y);
        }
        else if (mir_input_event_get_type(ievent) == mir_input_event_type_pointer)
        {
            auto pev = mir_input_event_get_pointer_input_event(ievent);
            x = mir_pointer_input_event_get_axis_value(pev, mir_pointer_input_axis_x);
            y = mir_pointer_input_event_get_axis_value(pev, mir_pointer_input_axis_y);
        }
        else
        {
            return;
        }
        context.make_current();
        program.draw(
            x/static_cast<float>(dimensions.width)*2.0 - 1.0,
            y/static_cast<float>(dimensions.height)*-2.0 + 1.0);
        context.swapbuffers();
    }

    Surface(Surface const&) = delete;
    Surface& operator=(Surface const&) = delete;
private:
    struct OutputDimensions
    {
        unsigned int const width;
        unsigned int const height;
    } const dimensions;


    std::function<void(MirSurface*)> const surface_deleter{
        [](MirSurface* surface)
        {
            mir_surface_release_sync(surface);
        }
    };
    std::unique_ptr<MirSurface, decltype(surface_deleter)> surface;
    Context context;
    RenderProgram program;

    OutputDimensions active_output_dimensions(MirConnection* connection)
    {
        unsigned int width{0};
        unsigned int height{0};
        auto display_config = mir_connection_create_display_config(connection);
        for (auto i = 0u; i < display_config->num_outputs; i++)
        {
            MirDisplayOutput const* out = display_config->outputs + i;
            if (out->used &&
                out->connected &&
                out->num_modes &&
                out->current_mode < out->num_modes)
            {
                width = out->modes[out->current_mode].horizontal_resolution;
                height = out->modes[out->current_mode].vertical_resolution;
                break;
            }
        }
        mir_display_config_destroy(display_config);
        if (width == 0 || height == 0)
            throw std::logic_error("could not determine display size");
        return {width, height};
    }

    MirSurface* create_surface(MirConnection* connection, OutputDimensions const& dim)
    {
        MirPixelFormat selected_format;
        unsigned int valid_formats{0};
        MirPixelFormat pixel_formats[mir_pixel_formats];
        mir_connection_get_available_surface_formats(connection, pixel_formats, mir_pixel_formats, &valid_formats);
        if (valid_formats == 0)
            throw std::runtime_error("no pixel formats for surface");
        selected_format = pixel_formats[0]; 
        //select an 8 bit opaque format if we can
        for(auto i = 0u; i < valid_formats; i++)
        {
            if (pixel_formats[i] == mir_pixel_format_xbgr_8888 ||
                pixel_formats[i] == mir_pixel_format_xrgb_8888)
            {
                selected_format = pixel_formats[i];
                break;
            }
        }

        auto deleter = [](MirSurfaceSpec *spec) { mir_surface_spec_release(spec); };
        std::unique_ptr<MirSurfaceSpec, decltype(deleter)> spec{
            mir_connection_create_spec_for_normal_surface(connection, dim.width, dim.height, selected_format),
            deleter
        };

        mir_surface_spec_set_name(spec.get(), __PRETTY_FUNCTION__);
        mir_surface_spec_set_buffer_usage(spec.get(), mir_buffer_usage_hardware);
        auto surface = mir_surface_create_sync(spec.get());
        MirEventDelegate delegate = {&on_event, this};
        mir_surface_set_event_handler(surface, &delegate);
        return surface;
    }
    static void on_event(MirSurface*, const MirEvent *event, void *context)
    {
        auto surface = reinterpret_cast<Surface*>(context);
        if (surface) surface->on_event(event);
    }
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

    Connection connection(socket_file);
    Surface surface(connection, swap_interval);

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
