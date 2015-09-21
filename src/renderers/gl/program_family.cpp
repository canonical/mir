/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
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
 * Authored by: Daniel van Vugt <daniel.van.vugt@canonical.com>
 */

#include "program_family.h"

#include <mutex>
#include <EGL/egl.h>

namespace mir
{
namespace renderer
{
namespace gl
{

void ProgramFamily::Shader::init(GLenum type, const GLchar* src)
{
    if (!id)
    {
        id = glCreateShader(type);
        if (id)
        {
            glShaderSource(id, 1, &src, NULL);
            glCompileShader(id);
            GLint ok;
            glGetShaderiv(id, GL_COMPILE_STATUS, &ok);
            if (!ok)
            {
                GLchar log[1024];
                glGetShaderInfoLog(id, sizeof log - 1, NULL, log);
                log[sizeof log - 1] = '\0';
                glDeleteShader(id);
                id = 0;
                throw std::runtime_error(std::string("Compile failed: ")+
                                         log + " for:\n" + src);
            }
        }
    }
}

ProgramFamily::~ProgramFamily() noexcept
{
    // shader and program lifetimes are managed manually, so that we don't
    // need any reference counting or to worry about how many copy constructions
    // might have been followed by destructor calls during container resizes.

    // Workaround for lp:1454201. Release any current GL context to avoid crashes
    // in the glDelete* functions that follow.
    eglMakeCurrent(eglGetCurrentDisplay(), EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

    for (auto& p : program)
    {
        if (p.second.id)
            glDeleteProgram(p.second.id);
    }

    for (auto& v : vshader)
    {
        if (v.second.id)
            glDeleteShader(v.second.id);
    }

    for (auto& f : fshader)
    {
        if (f.second.id)
            glDeleteShader(f.second.id);
    }
}

GLuint ProgramFamily::add_program(const GLchar* const vshader_src,
                                    const GLchar* const fshader_src)
{
    // Serialize calls - avoids segfault on multimonitor (see lp:1416482)
    // We need to serialize renderer creation because some GL calls used
    // during renderer construction that create unique resource ids
    // (e.g. glCreateProgram) are not thread-safe when the threads are
    // have the same or shared EGL contexts.
    static std::mutex lp1416482_mutex;
    std::lock_guard<decltype(lp1416482_mutex)> lock{lp1416482_mutex};

    auto& v = vshader[vshader_src];
    if (!v.id) v.init(GL_VERTEX_SHADER, vshader_src);

    auto& f = fshader[fshader_src];
    if (!f.id) f.init(GL_FRAGMENT_SHADER, fshader_src);

    auto& p = program[{v.id, f.id}];
    if (!p.id)
    {
        p.id = glCreateProgram();
        glAttachShader(p.id, v.id);
        glAttachShader(p.id, f.id);
        glLinkProgram(p.id);
        GLint ok;
        glGetProgramiv(p.id, GL_LINK_STATUS, &ok);
        if (!ok)
        {
            GLchar log[1024];
            glGetProgramInfoLog(p.id, sizeof log - 1, NULL, log);
            log[sizeof log - 1] = '\0';
            glDeleteShader(p.id);
            p.id = 0;
            throw std::runtime_error(std::string("Link failed: ")+log);
        }
    }

    return p.id;
}

}
}
}
