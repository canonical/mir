/*
 * Copyright © 2014 Canonical Ltd.
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

#include "mir/graphics/gl_program_factory.h"
#include "mir/graphics/gl_context.h"
#include "overlay_gl_compositor.h"

#define GLM_FORCE_RADIANS
#define GLM_PRECISION_MEDIUMP_FLOAT
#define GLM_HAS_INITIALIZER_LISTS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace mg = mir::graphics;
namespace mga = mir::graphics::android;
namespace geom = mir::geometry;

namespace
{
std::string const vertex_shader
{
    "attribute mat4 display_transform;\n"
    "void main() {\n"
    "   gl_Position = position;\n"
    "}\n"
};

std::string const fragment_shader
{
    "precision mediump float;\n"
    "void main() {\n"
    "   gl_FragColor = vec4(1.0, 1.0, 1.0, 1.0);\n"
    "}\n"
};

void set_display_transform(GLint uniform_loc, geom::Rectangle const& rect)
{
    glm::mat4 disp_transform(1.0);
    disp_transform = glm::translate(
                        disp_transform,
                        glm::vec3{rect.top_left.x.as_int(), rect.top_left.y.as_int(), 0.0});
    disp_transform = glm::scale(
                        disp_transform,
                        glm::vec3{1.0/rect.size.width.as_int(),
                                  1.0/rect.size.height.as_int(),
                                  1.0});
    glUniformMatrix4fv(uniform_loc, 1, GL_FALSE, glm::value_ptr(disp_transform));
}
}

mga::OverlayGLProgram::OverlayGLProgram(
    GLProgramFactory const& factory,
    GLContext const& context,
    geom::Rectangle const& screen_pos)
{
    context.make_current();
    program = factory.create_gl_program(vertex_shader, fragment_shader);

    display_transform_uniform = glGetUniformLocation(*program, "display_transform");
    set_display_transform(display_transform_uniform, screen_pos); 
    context.release_current();
}
