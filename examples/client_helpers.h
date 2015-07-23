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

#ifndef MIR_EXAMLPES_CLIENT_HELPERS_H_
#define MIR_EXAMLPES_CLIENT_HELPERS_H_

#include "mir_toolkit/mir_client_library.h"
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <memory>

namespace mir
{
namespace examples 
{
class Connection
{
public:
    Connection(char const* socket_file);
    ~Connection();
    operator MirConnection*();
    Connection(Connection const&) = delete;
    Connection& operator=(Connection const&) = delete;
private:
    MirConnection* connection;
};

class NormalSurface
{
public:
    NormalSurface(Connection& connection, unsigned int width, unsigned int height, bool prefers_alpha = false);
    
    operator MirSurface*() const;
private:
    MirSurface* create_surface(MirConnection* connection, unsigned int width, unsigned int height, bool prefers_alpha);
    std::function<void(MirSurface*)> const surface_deleter{
        [](MirSurface* surface) { mir_surface_release_sync(surface); }
    };
    std::unique_ptr<MirSurface, decltype(surface_deleter)> surface;
    NormalSurface(NormalSurface const&) = delete;
    NormalSurface& operator=(NormalSurface const&) = delete;
};

class Context
{
public:
    Context(Connection& connection, MirSurface* surface, int swap_interval);
    void make_current();
    void release_current();
    void swapbuffers();
    Context(Context const&) = delete;
    Context& operator=(Context const&) = delete;
private:
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
