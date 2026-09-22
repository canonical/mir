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
 */

#define MIR_LOG_COMPONENT "GLSceneDrawing"

#include "gl_scene_drawing.h"
#include "gl_program_factory.h"

#include <mir/gl/tessellation_helpers.h>
#include <mir/graphics/renderable.h>
#include <mir/graphics/texture.h>
#include <mir/graphics/transformation.h>
#include <mir/report_exception.h>

#define GLM_FORCE_RADIANS
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cmath>
#include <ranges>

namespace geom = mir::geometry;
namespace mg = mir::graphics;
namespace mgl = mir::gl;
namespace mrc = mir::renderer::common;

auto mrc::screen_to_gl_coords_for(geom::Rectangle const& viewport) -> glm::mat4
{
    auto screen_to_gl_coords = glm::translate(glm::mat4(1.0f), glm::vec3{-1.0f, 1.0f, 0.0f});

    /*
     * Perspective division is one thing that can't be done in a matrix
     * multiplication. It happens after the matrix multiplications. GL just
     * scales {x,y} by 1/w. So modify the final part of the projection matrix
     * to set w ([3]) to be the incoming z coordinate ([2]).
     */
    screen_to_gl_coords[2][3] = -1.0f;

    float const vertical_fov_degrees = 30.0f;
    float const near =
        (viewport.size.height.round_to<float>() / 2.0f) /
        std::tan((vertical_fov_degrees * M_PI / 180.0f) / 2.0f);
    float const far = -near;

    screen_to_gl_coords = glm::scale(screen_to_gl_coords,
            glm::vec3{2.0f / viewport.size.width.round_to<float>(),
                      -2.0f / viewport.size.height.round_to<float>(),
                      2.0f / (near - far)});
    screen_to_gl_coords = glm::translate(screen_to_gl_coords,
            glm::vec3{-viewport.top_left.x.round_to<float>(),
                      -viewport.top_left.y.round_to<float>(),
                      0.0f});

    return screen_to_gl_coords;
}

void mrc::set_letterboxed_gl_viewport(
    geom::Rectangle const& viewport,
    glm::mat4 const& display_transform,
    geom::Size output_size)
{
    auto transformed_viewport = display_transform *
                                glm::vec4(viewport.size.width.as_int(),
                                          viewport.size.height.as_int(), 0, 1);
    auto viewport_width = fabs(transformed_viewport[0]);
    auto viewport_height = fabs(transformed_viewport[1]);

    auto const output_width = output_size.width.as_value();
    auto const output_height = output_size.height.as_value();

    if (viewport_width > 0.0f && viewport_height > 0.0f &&
        output_width > 0 && output_height > 0)
    {
        GLint reduced_width = output_width, reduced_height = output_height;
        // if viewport_aspect_ratio >= output_aspect_ratio
        if (viewport_width * output_height >= output_width * viewport_height)
            reduced_height = static_cast<GLint>(output_width * viewport_height / viewport_width);
        else
            reduced_width = static_cast<GLint>(output_height * viewport_width / viewport_height);

        GLint offset_x = (output_width - reduced_width) / 2;
        GLint offset_y = (output_height - reduced_height) / 2;

        glViewport(offset_x, offset_y, reduced_width, reduced_height);
    }
}

void mrc::clear(GLfloat const (&colour)[4])
{
    glClearColor(colour[0], colour[1], colour[2], colour[3]);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glClear(GL_COLOR_BUFFER_BIT);
}

void mrc::tessellate(std::vector<mgl::Primitive>& primitives, mg::Renderable const& renderable)
{
    primitives.resize(1);
    primitives[0] = mgl::tessellate_renderable_into_rectangle(renderable, geom::Displacement{0,0});
}

namespace
{
template<typename T>
auto calc_scale(T logical, T physical) -> double
    requires requires{ logical.as_int(); physical.as_int(); }
{
    auto const l = logical.as_int();
    auto const p = physical.as_int();
    return (l > 0 && p > 0) ? static_cast<double>(p) / l : 1.0;
};
}

void mrc::draw(
    SceneContext const& context,
    mg::Renderable const& renderable,
    mg::gl::Texture& texture,
    mg::gl::ProgramFactory& factory,
    std::vector<mgl::Primitive> const& primitives)
{
    auto const clip_area = renderable.clip_area();
    if (clip_area)
    {
        glEnable(GL_SCISSOR_TEST);
        auto clip_x = clip_area.value().top_left.x.as_int();
        // The Y-coordinate is always relative to the top, so we make it relative to the bottom.
        auto clip_y = context.viewport.top_left.y.as_int() +
          context.viewport.size.height.as_int() -
          clip_area.value().top_left.y.as_int() -
          clip_area.value().size.height.as_int();
        glm::vec4 clip_pos(clip_x, clip_y, 0, 1);
        clip_pos = context.display_transform * clip_pos;

        // Calculate scale factor from logical viewport to physical output
        // When output has a scale factor (e.g. HiDPI), the viewport is in logical coordinates
        // but glScissor needs physical/framebuffer coordinates
        double const scale_x = calc_scale(context.viewport.size.width, context.output_size.width);
        double const scale_y = calc_scale(context.viewport.size.height, context.output_size.height);

        glScissor(
            static_cast<int>((clip_pos.x - static_cast<float>(context.viewport.top_left.x.as_int())) * scale_x),
            static_cast<int>(clip_pos.y * scale_y),
            static_cast<int>(clip_area.value().size.width.as_int() * scale_x),
            static_cast<int>(clip_area.value().size.height.as_int() * scale_y)
        );
    }

    // All the programs are held by the factory through its lifetime. Using pointers avoids
    // -Wdangling-reference.
    auto const* const prog =
        [&texture, &factory](bool alpha) -> gl::Renderer::Program const*
        {
                auto const& family = static_cast<ProgramFamily const&>(texture.shader(factory));
                if (alpha)
                {
                    return &family.alpha;
                }
                return &family.opaque;
        }(renderable.alpha() < 1.0f);

    glUseProgram(prog->id);
    if (prog->last_used_frameno != context.frameno)
    {   // Avoid reloading the screen-global uniforms on every renderable
        // TODO: We actually only need to bind these *once*, right? Not once per frame?
        prog->last_used_frameno = context.frameno;
        for (auto const [index, uniform] : prog->tex_uniforms | std::views::enumerate)
        {
            if (uniform != -1)
            {
                glUniform1i(uniform, static_cast<GLint>(index));
            }
        }
        glUniformMatrix4fv(prog->display_transform_uniform, 1, GL_FALSE,
                           glm::value_ptr(context.display_transform));
        glUniformMatrix4fv(prog->screen_to_gl_coords_uniform, 1, GL_FALSE,
                           glm::value_ptr(context.screen_to_gl_coords));
    }

    glActiveTexture(GL_TEXTURE0);

    auto const& rect = renderable.screen_position();
    GLfloat centrex = rect.top_left.x.round_to<GLfloat>() +
                      rect.size.width.round_to<GLfloat>() / 2.0f;
    GLfloat centrey = rect.top_left.y.round_to<GLfloat>() +
                      rect.size.height.round_to<GLfloat>() / 2.0f;
    glUniform2f(prog->centre_uniform, centrex, centrey);

    // Wayland surfaces may specify an orientation that matches the output
    // orientation. However, the surface is already rotated by the output's
    // orientation when we render it. To solve this, we need to unrotate the
    // surface using the inverse of its transform so that it appears upright.
    //
    // The inverse transformation is applied around the center of the rotated
    // buffer (e.g. if we have a 500x100 buffer that is rotated to the left,
    // then the renderable's dimensions will be 100x500 so we're rotating around
    // [50, 250]). Applying the inverse transformation unrotates the buffer,
    // but it fails to place it at the right position. Hence, we also need to
    // provid the "oriented centre" which represents the new centre after rotation.
    auto const orientation = renderable.orientation();
    if (orientation == mir_orientation_left || orientation == mir_orientation_right)
    {
        centrex = rect.top_left.x.round_to<GLfloat>() +
                        rect.size.height.round_to<GLfloat>() / 2.0f;
        centrey = rect.top_left.y.round_to<GLfloat>() +
                        rect.size.width.round_to<GLfloat>() / 2.0f;
    }
    glUniform2f(prog->oriented_centre, centrex, centrey);

    auto orientation_transform = glm::mat4(mg::inverse_transformation(orientation));
    glUniformMatrix4fv(prog->orientation_transform_uniform,
        1,
        GL_FALSE,
        glm::value_ptr(orientation_transform));

    auto const mirror_mode = renderable.mirror_mode();
    glm::mat4 transform = renderable.transformation()
        * glm::mat4(mg::transformation(mirror_mode)); // Unflip the buffer
    if (texture.layout() == mg::gl::Texture::Layout::TopRowFirst)
    {
        // GL textures have (0,0) at bottom-left rather than top-left
        // We have to invert this texture to get it the way up GL expects.
        transform *= glm::mat4{
            1.0, 0.0, 0.0, 0.0,
            0.0, -1.0, 0.0, 0.0,
            0.0, 0.0, 1.0, 0.0,
            0.0, 0.0, 0.0, 1.0
        };
    }

    glUniformMatrix4fv(prog->transform_uniform, 1, GL_FALSE,
                       glm::value_ptr(transform));

    if (prog->alpha_uniform >= 0)
        glUniform1f(prog->alpha_uniform, renderable.alpha());

    glEnableVertexAttribArray(prog->position_attr);
    glEnableVertexAttribArray(prog->texcoord_attr);

    // if we fail to load the texture, we need to carry on (part of lp:1629275)
    try
    {
        struct BlendSeparate  // Represents parameters of glBlendFuncSeparate()
        {
            GLenum src_rgb, dst_rgb, src_alpha, dst_alpha;
        };

        BlendSeparate client_blend;

        // These renderable method names could be better (see LP: #1236224)
        if (renderable.shaped())  // Client is RGBA:
        {
            client_blend = {GL_ONE, GL_ONE_MINUS_SRC_ALPHA,
                            GL_ONE, GL_ONE_MINUS_SRC_ALPHA};
        }
        else if (renderable.alpha() == 1.0f)  // RGBX and no window translucency:
        {
            client_blend = {GL_ONE,  GL_ZERO,
                            GL_ZERO, GL_ONE};  // Avoid using src_alpha!
        }
        else
        {   // Client is RGBX but we also have window translucency.
            // The texture alpha channel is possibly uninitialized so we must be
            // careful and avoid using SRC_ALPHA (LP: #1423462).
            client_blend = {GL_ONE,  GL_ONE_MINUS_CONSTANT_ALPHA,
                            GL_ZERO, GL_ONE};
            glBlendColor(0.0f, 0.0f, 0.0f, renderable.alpha());
        }

        for (auto const& p : primitives)
        {
            BlendSeparate blend;

            blend = client_blend;
            texture.bind();

            glVertexAttribPointer(prog->position_attr, 3, GL_FLOAT,
                                  GL_FALSE, sizeof(mgl::Vertex),
                                  &p.vertices[0].position);
            glVertexAttribPointer(prog->texcoord_attr, 2, GL_FLOAT,
                                  GL_FALSE, sizeof(mgl::Vertex),
                                  &p.vertices[0].texcoord);

            if (blend.dst_rgb == GL_ZERO)
            {
                glDisable(GL_BLEND);
            }
            else
            {
                glEnable(GL_BLEND);
                glBlendFuncSeparate(blend.src_rgb,   blend.dst_rgb,
                                    blend.src_alpha, blend.dst_alpha);
            }

            glDrawArrays(p.type, 0, p.nvertices);

            // We're done with the texture for now
            texture.add_syncpoint();
        }
    }
    catch (std::exception const& ex)
    {
        report_exception();
    }

    glDisableVertexAttribArray(prog->texcoord_attr);
    glDisableVertexAttribArray(prog->position_attr);
    if (clip_area)
    {
        glDisable(GL_SCISSOR_TEST);
    }
}
