/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Daniel van Vugt <daniel.van.vugt@canonical.com>
 *              Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_GRAPHICS_GL_PRIMITIVE_H_
#define MIR_GRAPHICS_GL_PRIMITIVE_H_

#include <GLES2/gl2.h>
#include <vector>

namespace mir
{
namespace graphics
{
struct GLVertex
{
    GLfloat position[3];
    GLfloat texcoord[2];
};

struct GLPrimitive
{
    GLenum type; // GL_TRIANGLE_STRIP, GL_TRIANGLE_FAN, GL_TRIANGLES etc
    GLuint tex_id;  // GL texture ID (or 0 to represent the surface itself)
    std::vector<GLVertex> vertices;
};
}
}
#endif /* MIR_GRAPHICS_GL_PRIMITIVE_H_ */
