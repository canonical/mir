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

#include "mir/gl/program_factory.h"
#include "mir/gl/texture.h"
#include "mir/gl/tessellation_helpers.h"
#include "mir/graphics/gl_context.h"
#include "hwc_fallback_gl_renderer.h"
#include "swapping_gl_context.h"
#include "buffer.h"

#define GLM_FORCE_RADIANS
#define GLM_PRECISION_MEDIUMP_FLOAT
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace mg = mir::graphics;
namespace mgl = mir::gl;
namespace mga = mir::graphics::android;
namespace geom = mir::geometry;

namespace
{
std::string const vertex_shader
{
    "attribute vec3 position;\n"
    "attribute vec2 texcoord;\n"
    "uniform mat4 display_transform;\n"
    "varying vec2 v_texcoord;\n"
    "void main() {\n"
    "   gl_Position = display_transform * vec4(position, 1.0);\n"
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

mga::HWCFallbackGLRenderer::HWCFallbackGLRenderer(
    gl::ProgramFactory const& factory,
    mg::GLContext const& context,
    geom::Rectangle const& screen_pos)
{
    context.make_current();
    program = factory.create_gl_program(vertex_shader, fragment_shader);
    texture_cache = factory.create_texture_cache();

    glUseProgram(*program);

    auto display_transform_uniform = glGetUniformLocation(*program, "display_transform");
    set_display_transform(display_transform_uniform, screen_pos); 

    position_attr = glGetAttribLocation(*program, "position");
    texcoord_attr = glGetAttribLocation(*program, "texcoord");

    auto tex_loc = glGetUniformLocation(*program, "tex");
    glUniform1i(tex_loc, 0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glUseProgram(0);
    context.release_current();
}

void mga::HWCFallbackGLRenderer::render(
    RenderableList const& renderlist, geom::Displacement offset, SwappingGLContext const& context) const
{
    glUseProgram(*program);

    /* NOTE: some HWC implementations rely on the framebuffer target layer
     * being cleared to transparent black. eg, in mixed-mode composition,
     * krillin actually arranges the fb_target in the topmost level of its
     * display controller and relies on blending to make the overlays appear
     * /under/ the gl layer. (lp: #1378326)
     */
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glClear(GL_COLOR_BUFFER_BIT);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glEnableVertexAttribArray(position_attr);
    glEnableVertexAttribArray(texcoord_attr);

    for(auto const& renderable : renderlist)
    {
        if (renderable->shaped())  // TODO: support alpha() in future
            glEnable(GL_BLEND);
        else
            glDisable(GL_BLEND);

        auto const primitive = mgl::tessellate_renderable_into_rectangle(*renderable, offset);
        glVertexAttribPointer(position_attr, 3, GL_FLOAT, GL_FALSE, sizeof(mgl::Vertex),
                              &primitive.vertices[0].position);
        //TODO: (kdub) scaling or pi/2 rotation eventually. for now, all quads get same texcoords
        glVertexAttribPointer(texcoord_attr, 2, GL_FLOAT, GL_FALSE, sizeof(mgl::Vertex),
                              &primitive.vertices[0].texcoord);

        texture_cache->load(*renderable)->bind();
        glDrawArrays(primitive.type, 0, primitive.nvertices);
    }

    glDisableVertexAttribArray(texcoord_attr);
    glDisableVertexAttribArray(position_attr);
    context.swap_buffers();
    texture_cache->drop_unused();
    glUseProgram(0);
}
