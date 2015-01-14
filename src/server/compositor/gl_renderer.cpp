/* Copyright © 2013 Canonical Ltd.
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
 * Authored By: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#define MIR_LOG_COMPONENT "GL"
#include "mir/compositor/gl_renderer.h"
#include "mir/compositor/buffer_stream.h"
#include "mir/compositor/destination_alpha.h"
#include "mir/graphics/renderable.h"
#include "mir/graphics/buffer.h"
#include "mir/graphics/gl_texture_cache.h"
#include "mir/graphics/gl_texture.h"
#include "mir/graphics/tessellation_helpers.h"
#include "mir/log.h"

#define GLM_FORCE_RADIANS
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <boost/throw_exception.hpp>
#include <stdexcept>
#include <cmath>

namespace mg = mir::graphics;
namespace mc = mir::compositor;
namespace geom = mir::geometry;

namespace
{

enum
{
    default_program_index = 0,
    alpha_program_index = 1
};

const GLchar vshader[] =
{
    "attribute vec3 position;\n"
    "attribute vec2 texcoord;\n"
    "uniform mat4 screen_to_gl_coords;\n"
    "uniform mat4 display_transform;\n"
    "uniform mat4 transform;\n"
    "uniform vec2 centre;\n"
    "varying vec2 v_texcoord;\n"
    "void main() {\n"
    "   vec4 mid = vec4(centre, 0.0, 0.0);\n"
    "   vec4 transformed = (transform * (vec4(position, 1.0) - mid)) + mid;\n"
    "   gl_Position = display_transform * screen_to_gl_coords * transformed;\n"
    "   v_texcoord = texcoord;\n"
    "}\n"
};

const GLchar alpha_fshader[] =
{
    "precision mediump float;\n"
    "uniform sampler2D tex;\n"
    "uniform float alpha;\n"
    "varying vec2 v_texcoord;\n"
    "void main() {\n"
    "   vec4 frag = texture2D(tex, v_texcoord);\n"
    "   gl_FragColor = alpha*frag;\n"
    "}\n"
};

const GLchar default_fshader[] =  // Should be faster than blending in theory
{
    "precision mediump float;\n"
    "uniform sampler2D tex;\n"
    "varying vec2 v_texcoord;\n"
    "void main() {\n"
    "   gl_FragColor = texture2D(tex, v_texcoord);\n"
    "}\n"
};

}

mc::GLRenderer::Program::Program(GLuint program_id)
{
    id = program_id;
    position_attr = glGetAttribLocation(id, "position");
    texcoord_attr = glGetAttribLocation(id, "texcoord");
    tex_uniform = glGetUniformLocation(id, "tex");
    centre_uniform = glGetUniformLocation(id, "centre");
    display_transform_uniform = glGetUniformLocation(id, "display_transform");
    transform_uniform = glGetUniformLocation(id, "transform");
    screen_to_gl_coords_uniform = glGetUniformLocation(id, "screen_to_gl_coords");
    alpha_uniform = glGetUniformLocation(id, "alpha");
}

mc::GLRenderer::GLRenderer(
    std::unique_ptr<mg::GLTextureCache> && texture_cache, 
    geom::Rectangle const& display_area,
    DestinationAlpha dest_alpha)
    : clear_color{0.0f, 0.0f, 0.0f, 1.0f},
      programs{family.add_program(vshader, default_fshader),
               family.add_program(vshader, alpha_fshader)},
      texture_cache(std::move(texture_cache)),
      rotation(NAN), // ensure the first set_rotation succeeds
      dest_alpha(dest_alpha)
{
    struct {GLenum id; char const* label;} const glstrings[] =
    {
        {GL_VENDOR,   "vendor"},
        {GL_RENDERER, "renderer"},
        {GL_VERSION,  "version"},
        {GL_SHADING_LANGUAGE_VERSION,  "SL version"},
    };

    for (auto& s : glstrings)
    {
        auto val = reinterpret_cast<char const*>(glGetString(s.id));
        if (!val) val = "";
        mir::log_info("%s: %s", s.label, val);
    }

    for (auto& p : programs)
    {
        glUseProgram(p.id);
        glUniform1i(p.tex_uniform, 0);
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glUseProgram(0);

    set_viewport(display_area);
    set_rotation(0.0f);

    if (dest_alpha != DestinationAlpha::opaque)
        clear_color[3] = 0.0f;
}

void mc::GLRenderer::tessellate(std::vector<mg::GLPrimitive>& primitives,
                                mg::Renderable const& renderable) const
{
    primitives.resize(1);
    primitives[0] = mg::tessellate_renderable_into_rectangle(renderable);
}

void mc::GLRenderer::render(mg::RenderableList const& renderables) const
{
    glClearColor(clear_color[0], clear_color[1], clear_color[2], clear_color[3]);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glClear(GL_COLOR_BUFFER_BIT);

    if (dest_alpha == DestinationAlpha::opaque)
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);

    for (auto const& r : renderables)
        render(*r);

    texture_cache->drop_unused();
}

void mc::GLRenderer::render(mg::Renderable const& renderable) const
{
    const Program* prog = &programs[default_program_index];

    if (renderable.alpha() < 1.0f)
    {
        prog = &programs[alpha_program_index];
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    }
    else if (renderable.shaped())
    {
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    }
    else
    {
        glDisable(GL_BLEND);
    }

    glUseProgram(prog->id);
    glActiveTexture(GL_TEXTURE0);

    auto const& rect = renderable.screen_position();
    GLfloat centrex = rect.top_left.x.as_int() +
                      rect.size.width.as_int() / 2.0f;
    GLfloat centrey = rect.top_left.y.as_int() +
                      rect.size.height.as_int() / 2.0f;
    glUniform2f(prog->centre_uniform, centrex, centrey);

    glUniformMatrix4fv(prog->transform_uniform, 1, GL_FALSE,
                       glm::value_ptr(renderable.transformation()));

    if (prog->alpha_uniform >= 0)
        glUniform1f(prog->alpha_uniform, renderable.alpha());

    /* Draw */
    glEnableVertexAttribArray(prog->position_attr);
    glEnableVertexAttribArray(prog->texcoord_attr);

    primitives.clear();
    tessellate(primitives, renderable);

    auto surface_tex = texture_cache->load(renderable);

    for (auto const& p : primitives)
    {
        // Note a primitive tex_id of zero means use the surface texture,
        // which is what you normally want. Other textures could be used
        // in decorations etc.
        if (p.tex_id) //tessalate() can be overridden, and that code can set to nonzero  
            glBindTexture(GL_TEXTURE_2D, p.tex_id);
        else
            surface_tex->bind();

        glVertexAttribPointer(prog->position_attr, 3, GL_FLOAT,
                              GL_FALSE, sizeof(mg::GLVertex),
                              &p.vertices[0].position);
        glVertexAttribPointer(prog->texcoord_attr, 2, GL_FLOAT,
                              GL_FALSE, sizeof(mg::GLVertex),
                              &p.vertices[0].texcoord);

        glDrawArrays(p.type, 0, p.nvertices);
    }

    glDisableVertexAttribArray(prog->texcoord_attr);
    glDisableVertexAttribArray(prog->position_attr);
}

void mc::GLRenderer::set_viewport(geometry::Rectangle const& rect)
{
    if (rect == viewport)
        return;

    /*
     * Create and set screen_to_gl_coords transformation matrix.
     * The screen_to_gl_coords matrix transforms from the screen coordinate system
     * (top-left is (0,0), bottom-right is (W,H)) to the normalized GL coordinate system
     * (top-left is (-1,1), bottom-right is (1,-1))
     */
    glm::mat4 screen_to_gl_coords = glm::translate(glm::mat4(1.0f), glm::vec3{-1.0f, 1.0f, 0.0f});

    /*
     * Perspective division is one thing that can't be done in a matrix
     * multiplication. It happens after the matrix multiplications. GL just
     * scales {x,y} by 1/w. So modify the final part of the projection matrix
     * to set w ([3]) to be the incoming z coordinate ([2]).
     */
    screen_to_gl_coords[2][3] = -1.0f;

    float const vertical_fov_degrees = 30.0f;
    float const near =
        (rect.size.height.as_float() / 2.0f) /
        std::tan((vertical_fov_degrees * M_PI / 180.0f) / 2.0f);
    float const far = -near;

    screen_to_gl_coords = glm::scale(screen_to_gl_coords,
            glm::vec3{2.0f / rect.size.width.as_float(),
                      -2.0f / rect.size.height.as_float(),
                      2.0f / (near - far)});
    screen_to_gl_coords = glm::translate(screen_to_gl_coords,
            glm::vec3{-rect.top_left.x.as_float(),
                      -rect.top_left.y.as_float(),
                      0.0f});

    for (auto& p : programs)
    {
        glUseProgram(p.id);
        glUniformMatrix4fv(p.screen_to_gl_coords_uniform, 1, GL_FALSE,
                           glm::value_ptr(screen_to_gl_coords));
    }
    glUseProgram(0);

    viewport = rect;
}

void mc::GLRenderer::set_rotation(float degrees)
{
    if (degrees == rotation)
        return;

    float rad = degrees * M_PI / 180.0f;
    GLfloat cos = cosf(rad);
    GLfloat sin = sinf(rad);
    GLfloat rot[16] = {cos,  sin,  0.0f, 0.0f,
                       -sin, cos,  0.0f, 0.0f,
                       0.0f, 0.0f, 1.0f, 0.0f,
                       0.0f, 0.0f, 0.0f, 1.0f};

    for (auto& p : programs)
    {
        glUseProgram(p.id);
        glUniformMatrix4fv(p.display_transform_uniform, 1, GL_FALSE, rot);
    }
    glUseProgram(0);

    rotation = degrees;
}

void mc::GLRenderer::suspend()
{
    texture_cache->invalidate();
}

mc::DestinationAlpha mc::GLRenderer::destination_alpha() const
{
    return dest_alpha;
}
