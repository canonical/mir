/*
 * Copyright © 2017 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_RENDERER_GL_EGL_ACCESS_H_
#define MIR_RENDERER_GL_EGL_ACCESS_H_

#include <EGL/egl.h>

namespace mir
{
namespace renderer
{
namespace gl
{

class EGLAccess
{
public:
    virtual ~EGLAccess() = default;
    virtual EGLNativeDisplayType egl_native_display() const = 0;

protected:
    EGLAccess() = default;
    EGLAccess(EGLAccess const&) = delete;
    EGLAccess& operator=(EGLAccess const&) = delete;
};

}
}
}

#endif
