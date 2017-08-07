/*
 * Copyright © 2015-2017 Canonical Ltd.
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

#include "client_helpers.h"
#include "mir_toolkit/mir_client_library.h"
#include <unistd.h>
#include <signal.h>

namespace me = mir::examples;

me::Connection::Connection(char const* socket_file, char const* name)
    : connection{mir_connect_sync(socket_file, name)}
{
    if (!mir_connection_is_valid(connection))
        throw std::runtime_error(
            std::string("could not connect to server: ") +
            mir_connection_get_error_message(connection));
}

me::Connection::Connection(char const* socket_file) :
    Connection(socket_file, __PRETTY_FUNCTION__)
{
}

me::Connection::~Connection()
{
    mir_connection_release(connection);
}

me::Connection::operator MirConnection*()
{
    return connection;
}

me::NormalWindow::NormalWindow(me::Connection& connection, unsigned int width, unsigned int height) :
    window{create_window(connection, width, height, nullptr), &mir_window_release_sync}
{
}

mir::examples::NormalWindow::NormalWindow(
    Connection& connection, unsigned int width, unsigned int height, MirRenderSurface* surface) :
    window{create_window(connection, width, height, surface), &mir_window_release_sync}
{
}

mir::examples::NormalWindow::NormalWindow(
    mir::examples::Connection& connection, unsigned int width, unsigned int height, mir::examples::Context& eglcontext) :
    NormalWindow(connection, width, height, eglcontext.mir_surface())
{
}

me::NormalWindow::operator MirWindow*() const
{
    return window.get();
}

namespace
{
void handle_window_event(MirWindow* window, MirEvent const* event, void* context)
{
    auto const surface = (MirRenderSurface*)context;

    switch (mir_event_get_type(event))
    {
    case mir_event_type_resize:
    {
        MirResizeEvent const* resize = mir_event_get_resize_event(event);
        int const new_width = mir_resize_event_get_width(resize);
        int const new_height = mir_resize_event_get_height(resize);

        mir_render_surface_set_size(surface, new_width, new_height);
        MirWindowSpec* spec = mir_create_window_spec(mir_window_get_connection(window));
        mir_window_spec_add_render_surface(spec, surface, new_width, new_height, 0, 0);
        mir_window_apply_spec(window, spec);
        mir_window_spec_release(spec);
        break;
    }

    case mir_event_type_close_window:
        kill(getpid(), SIGTERM);
        break;

    default:
        break;
    }
}
}

MirWindow* me::NormalWindow::create_window(
    MirConnection* connection, unsigned int width, unsigned int height, MirRenderSurface* surface)
{
    std::unique_ptr<MirWindowSpec, decltype(&mir_window_spec_release)> spec{
        mir_create_normal_window_spec(connection, width, height),
        &mir_window_spec_release
    };

    mir_window_spec_set_name(spec.get(), __PRETTY_FUNCTION__);

    if (surface)
    {
        mir_window_spec_add_render_surface(spec.get(), surface, width, height, 0, 0);
        mir_window_spec_set_event_handler(spec.get(), &handle_window_event, surface);
    }

    auto window = mir_create_window_sync(spec.get());
    return window;
}

me::Context::Context(Connection& connection, int swap_interval, unsigned int width, unsigned int height) :
    rsurface{mir_connection_create_render_surface_sync(connection, width, height), &mir_render_surface_release},
    native_display(connection),
    native_window(reinterpret_cast<EGLNativeWindowType>(rsurface.get())),
    display(native_display),
    config(chooseconfig(display.disp)),
    surface(display.disp, config, native_window),
    context(display.disp, config)
{
    make_current();
    eglSwapInterval(display.disp, swap_interval);
}

void me::Context::make_current()
{
    if (eglMakeCurrent(display.disp, surface.surface, surface.surface, context.context) == EGL_FALSE)
        throw(std::runtime_error("could not makecurrent"));
}

void me::Context::release_current()
{
    if (eglMakeCurrent(display.disp, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT) == EGL_FALSE)
        throw(std::runtime_error("could not makecurrent"));
}
void me::Context::swapbuffers()
{
    if (eglSwapBuffers(display.disp, surface.surface) == EGL_FALSE)
        throw(std::runtime_error("could not swapbuffers"));
}

EGLConfig me::Context::chooseconfig(EGLDisplay disp)
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

me::Context::Display::Display(EGLNativeDisplayType native) :
    disp(eglGetDisplay(native))
{
    int major{0}, minor{0};
    if (disp == EGL_NO_DISPLAY)
        throw std::runtime_error("no egl display");
    if (eglInitialize(disp, &major, &minor) != EGL_TRUE || major != 1 || minor != 4)
        throw std::runtime_error("could not init egl");
}

me::Context::Display::~Display()
{
    eglTerminate(disp);
}

me::Context::Surface::Surface(EGLDisplay display, EGLConfig config, EGLNativeWindowType native_window) :
    disp(display),
    surface(eglCreateWindowSurface(disp, config, native_window, NULL))
{
    if (surface == EGL_NO_SURFACE)
        throw std::runtime_error("could not create egl surface");
}

me::Context::Surface::~Surface()
{
    if (eglGetCurrentSurface(EGL_DRAW) == surface)
        eglMakeCurrent(disp, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroySurface(disp, surface);
}

me::Context::EglContext::EglContext(EGLDisplay disp, EGLConfig config) :
    disp(disp),
    context(eglCreateContext(disp, config, EGL_NO_CONTEXT, context_attribs))
{
    if (context == EGL_NO_CONTEXT)
        throw std::runtime_error("could not create egl context");
}

me::Context::EglContext::~EglContext()
{
    eglDestroyContext(disp, context);
}

me::Shader::Shader(GLchar const* const* src, GLuint type) :
    shader(glCreateShader(type))
{
    glShaderSource(shader, 1, src, 0);
    glCompileShader(shader);
}

me::Shader::~Shader()
{
    glDeleteShader(shader);
}

me::Program::Program(Shader& vertex, Shader& fragment) :
    program(glCreateProgram())
{
    glAttachShader(program, vertex.shader);
    glAttachShader(program, fragment.shader);
    glLinkProgram(program);
}

me::Program::~Program()
{
    glDeleteProgram(program);
}
