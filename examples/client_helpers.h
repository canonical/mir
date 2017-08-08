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

#ifndef MIR_EXAMPLES_CLIENT_HELPERS_H_
#define MIR_EXAMPLES_CLIENT_HELPERS_H_

#include "mir_toolkit/mir_client_library.h"
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <memory>
#include <functional>

namespace mir
{
namespace examples 
{
class Connection
{
public:
    Connection(char const* socket_file);
    Connection(char const* socket_file, const char* name);
    ~Connection();
    operator MirConnection*();
    Connection(Connection const&) = delete;
    Connection& operator=(Connection const&) = delete;
private:
    MirConnection* connection;
};

class Context
{
public:
    Context(Connection& connection, int swap_interval, unsigned int width, unsigned int height);
    void make_current();
    void release_current();
    void swapbuffers();
    MirRenderSurface* mir_surface() const { return rsurface.get(); }
    Context(Context const&) = delete;
    Context& operator=(Context const&) = delete;
private:
    std::unique_ptr<MirRenderSurface, decltype(&mir_render_surface_release)> rsurface;
    EGLConfig chooseconfig(EGLDisplay disp);
    EGLNativeDisplayType native_display;
    EGLNativeWindowType native_window;
    struct Display
    {
        Display(EGLNativeDisplayType native);
        ~Display();
        EGLDisplay disp;
    } display;
    EGLConfig config;
    struct Surface
    {
        Surface(EGLDisplay display, EGLConfig config, EGLNativeWindowType native_window);
        ~Surface();
        EGLDisplay disp;
        EGLSurface surface;
    } surface;
    struct EglContext
    {
        EglContext(EGLDisplay disp, EGLConfig config);
        ~EglContext();
        EGLint context_attribs[3] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
        EGLDisplay disp;
        EGLContext context;
    } context;
};

class NormalWindow
{
public:
    NormalWindow(Connection& connection, unsigned int width, unsigned int height);
    NormalWindow(Connection& connection, unsigned int width, unsigned int height, Context& eglcontext);
    NormalWindow(Connection& connection, unsigned int width, unsigned int height, MirRenderSurface* surface);

    operator MirWindow*() const;
private:
    MirWindow* create_window(
        MirConnection* connection, unsigned int width, unsigned int height, MirRenderSurface* surface);
    std::unique_ptr<MirWindow, decltype(&mir_window_release_sync)> window;
    NormalWindow(NormalWindow const&) = delete;
    NormalWindow& operator=(NormalWindow const&) = delete;
};


struct Shader
{
    Shader(GLchar const* const* src, GLuint type);
    ~Shader();
    GLuint shader;
};

struct Program
{
    Program(Shader& vertex, Shader& fragment);
    ~Program();
    GLuint program;
};
}
}
#endif /* MIR_EXAMLPES_CLIENT_HELPERS_H_ */
