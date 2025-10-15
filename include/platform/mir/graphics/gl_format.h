/*
 * Copyright © Canonical Ltd.
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
 */

#ifndef MIR_GRAPHICS_COMMON_GL_FORMAT_H_
#define MIR_GRAPHICS_COMMON_GL_FORMAT_H_
#include <GLES2/gl2.h>
#include <mir_toolkit/common.h>

namespace mir
{
namespace graphics
{
bool get_gl_pixel_format(MirPixelFormat mir_format,
                         GLenum& gl_format, GLenum& gl_type);
}
}
#endif /* MIR_GRAPHICS_COMMON_GL_FORMAT_H_ */
