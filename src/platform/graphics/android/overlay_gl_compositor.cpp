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
#include "gl_context.h"
#include "buffer.h"

#define GLM_FORCE_RADIANS
#define GLM_PRECISION_MEDIUMP_FLOAT
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
    "attribute vec2 position;\n"
    "attribute vec2 texcoord;\n"
    "uniform mat4 display_transform;\n"
    "varying vec2 v_texcoord;\n"
    "void main() {\n"
    "   gl_Position = display_transform * vec4(position, 0.0, 1.0);\n"
    "   v_texcoord = texcoord;\n"
    "}\n"
};

std::string const fragment_shader
{
    "precision mediump float;\n"
    "uniform sampler2D tex;\n"
    "varying vec2 v_texcoord;\n"
    "void main() {\n"
    "   gl_FragColor = texture2D(tex, v_texcoord);\n"
    "}\n"
};

void set_display_transform(GLint uniform_loc, geom::Rectangle const& rect)
{
    glm::mat4 disp_transform(1.0);

    disp_transform = glm::translate(disp_transform, glm::vec3{-1.0, 1.0, 0.0});
    disp_transform = glm::scale(
                        disp_transform,
                        glm::vec3{2.0/rect.size.width.as_float(),
                                  -2.0/rect.size.height.as_float(),
                                  1.0});
    glUniformMatrix4fv(uniform_loc, 1, GL_FALSE, glm::value_ptr(disp_transform));
}
}

mga::OverlayGLProgram::OverlayGLProgram(
    GLProgramFactory const& factory,
    mg::GLContext const& context,
    geom::Rectangle const& screen_pos)
{
    context.make_current();
    program = factory.create_gl_program(vertex_shader, fragment_shader);

    glUseProgram(*program);

    auto display_transform_uniform = glGetUniformLocation(*program, "display_transform");
    set_display_transform(display_transform_uniform, screen_pos); 

    position_attr = glGetAttribLocation(*program, "position");
    texcoord_attr = glGetAttribLocation(*program, "texcoord");

    glGenTextures(1, &tex_id);
    glBindTexture(GL_TEXTURE_2D, tex_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    auto tex_loc = glGetUniformLocation(*program, "tex");
    glUniform1i(tex_loc, 0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glUseProgram(0);
    context.release_current();
}

namespace
{
size_t const num_vertices{4};
size_t const vertex_order{2};
GLfloat const texcoords[]{
    0.0f, 0.0f,
    0.0f, 1.0f,
    1.0f, 0.0f,
    1.0f, 1.0f
};
}

void mga::OverlayGLProgram::render(
    RenderableList const& renderlist, SwappingGLContext const& context)
{
    glUseProgram(*program);

    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);
    
    glEnableVertexAttribArray(position_attr);
    glEnableVertexAttribArray(texcoord_attr);

    //TODO: (kdub) scaling or pi/2 rotation eventually. for now, all quads get same texcoords
    glVertexAttribPointer(texcoord_attr, vertex_order, GL_FLOAT, GL_FALSE, 0, texcoords);
    glBindTexture(GL_TEXTURE_2D, tex_id);

    for(auto const& renderable : renderlist)
    {
        auto screen_pos = renderable->screen_position();
        GLfloat vertices[num_vertices*vertex_order]{
            screen_pos.top_left.x.as_float(), screen_pos.top_left.y.as_float(),
            screen_pos.top_right().x.as_float(), screen_pos.top_right().y.as_float(),
            screen_pos.bottom_left().x.as_float(), screen_pos.bottom_left().y.as_float(),
            screen_pos.bottom_right().x.as_float(), screen_pos.bottom_right().y.as_float(),
        };

        renderable->buffer(this)->bind_to_texture();
        glVertexAttribPointer(position_attr, vertex_order, GL_FLOAT, GL_FALSE, 0, vertices);

        glDrawArrays(GL_TRIANGLE_STRIP, 0, num_vertices);
    }

    glDisableVertexAttribArray(texcoord_attr);
    glDisableVertexAttribArray(position_attr);
    context.swap_buffers();
    glUseProgram(0);
}
