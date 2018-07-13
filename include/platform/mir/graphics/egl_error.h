/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_GRAPHICS_EGL_ERROR_H_
#define MIR_GRAPHICS_EGL_ERROR_H_

#include <system_error>
#include <EGL/egl.h>
#include <GLES2/gl2.h>

namespace mir
{
namespace graphics
{

std::error_category const& egl_category();

inline auto egl_error(std::string const& msg) -> std::system_error
{
    return std::system_error{eglGetError(), egl_category(), msg};
}

std::error_category const& gl_category();

inline auto gl_error(std::string const& msg) -> std::system_error
{
    return std::system_error{static_cast<int>(glGetError()), gl_category(), msg};
}

}
}

#endif
