/* Copyright © 2013, 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
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
 *              Daniel van Vugt <daniel.van.vugt@canonical.com>
 */

#define MIR_LOG_COMPONENT "GLRenderer"

#include "renderer.h"
#include "mir/compositor/buffer_stream.h"
#include "mir/gl/default_program_factory.h"
#include "mir/graphics/renderable.h"
#include "mir/graphics/buffer.h"
#include "mir/graphics/display_buffer.h"
#include "mir/gl/tessellation_helpers.h"
#include "mir/gl/texture_cache.h"
#include "mir/gl/texture.h"
#include "mir/log.h"
#include "mir/report_exception.h"

#define GLM_FORCE_RADIANS
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <EGL/egl.h>

#include <boost/throw_exception.hpp>
#include <stdexcept>
#include <cmath>

namespace mg = mir::graphics;
namespace mgl = mir::gl;
namespace mrg = mir::renderer::gl;
namespace geom = mir::geometry;

mrg::CurrentRenderTarget::CurrentRenderTarget(mg::DisplayBuffer* display_buffer)
    : render_target{
        dynamic_cast<renderer::gl::RenderTarget*>(display_buffer->native_display_buffer())}
{
    if (!render_target)
        BOOST_THROW_EXCEPTION(std::logic_error("DisplayBuffer does not support GL rendering"));

    ensure_current();
}

mrg::CurrentRenderTarget::~CurrentRenderTarget()
{
    render_target->release_current();
}

void mrg::CurrentRenderTarget::ensure_current()
{
    render_target->make_current();
}

void mrg::CurrentRenderTarget::bind()
{
    ensure_current();
    render_target->bind();
}

void mrg::CurrentRenderTarget::swap_buffers()
{
    render_target->swap_buffers();
}

const GLchar* const mrg::Renderer::vshader =
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

const GLchar* const mrg::Renderer::alpha_fshader =
{
    "#ifdef GL_ES\n"
    "precision mediump float;\n"
    "#endif\n"
    "uniform sampler2D tex;\n"
    "uniform float alpha;\n"
    "varying vec2 v_texcoord;\n"
    "void main() {\n"
    "   vec4 frag = texture2D(tex, v_texcoord);\n"
    "   gl_FragColor = alpha*frag;\n"
    "}\n"
};

const GLchar* const mrg::Renderer::default_fshader =
{   // This is the fastest fragment shader. Use it when you can.
    "#ifdef GL_ES\n"
    "precision mediump float;\n"
    "#endif\n"
    "uniform sampler2D tex;\n"
    "varying vec2 v_texcoord;\n"
    "void main() {\n"
    "   gl_FragColor = texture2D(tex, v_texcoord);\n"
    "}\n"
};

mrg::Renderer::Program::Program(GLuint program_id)
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

mrg::Renderer::Renderer(graphics::DisplayBuffer& display_buffer)
    : render_target(&display_buffer),
      clear_color{0.0f, 0.0f, 0.0f, 0.0f},
      default_program(family.add_program(vshader, default_fshader)),
      alpha_program(family.add_program(vshader, alpha_fshader)),
      texture_cache(mgl::DefaultProgramFactory().create_texture_cache())
{
    eglBindAPI(MIR_SERVER_EGL_OPENGL_API);
    EGLDisplay disp = eglGetCurrentDisplay();
    if (disp != EGL_NO_DISPLAY)
    {
        struct {GLint id; char const* label;} const eglstrings[] =
        {
            {EGL_VENDOR,      "EGL vendor"},
            {EGL_VERSION,     "EGL version"},
            {EGL_CLIENT_APIS, "EGL client APIs"},
        };
        for (auto& s : eglstrings)
        {
            auto val = eglQueryString(disp, s.id);
            mir::log_info(std::string(s.label) + ": " + (val ? val : ""));
        }
    }

    struct {GLenum id; char const* label;} const glstrings[] =
    {
        {GL_VENDOR,   "GL vendor"},
        {GL_RENDERER, "GL renderer"},
        {GL_VERSION,  "GL version"},
        {GL_SHADING_LANGUAGE_VERSION,  "GLSL version"},
    };

    for (auto& s : glstrings)
    {
        auto val = reinterpret_cast<char const*>(glGetString(s.id));
        mir::log_info(std::string(s.label) + ": " + (val ? val : ""));
    }

    GLint max_texture_size = 0;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_texture_size);
    mir::log_info("GL max texture size = %d", max_texture_size);

    GLint rbits = 0, gbits = 0, bbits = 0, abits = 0, dbits = 0, sbits = 0;
    glGetIntegerv(GL_RED_BITS, &rbits);
    glGetIntegerv(GL_GREEN_BITS, &gbits);
    glGetIntegerv(GL_BLUE_BITS, &bbits);
    glGetIntegerv(GL_ALPHA_BITS, &abits);
    glGetIntegerv(GL_DEPTH_BITS, &dbits);
    glGetIntegerv(GL_STENCIL_BITS, &sbits);
    mir::log_info("GL framebuffer bits: RGBA=%d%d%d%d, depth=%d, stencil=%d",
                  rbits, gbits, bbits, abits, dbits, sbits);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    set_viewport(display_buffer.view_area());
}

mrg::Renderer::~Renderer()
{
    render_target.ensure_current();
}

void mrg::Renderer::tessellate(std::vector<mgl::Primitive>& primitives,
                                mg::Renderable const& renderable) const
{
    primitives.resize(1);
    primitives[0] = mgl::tessellate_renderable_into_rectangle(renderable, geom::Displacement{0,0});
}

void mrg::Renderer::render(mg::RenderableList const& renderables) const
{
    render_target.bind();

    glClearColor(clear_color[0], clear_color[1], clear_color[2], clear_color[3]);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glClear(GL_COLOR_BUFFER_BIT);

    ++frameno;
    for (auto const& r : renderables)
        draw(*r, r->alpha() < 1.0f ? alpha_program : default_program);

    render_target.swap_buffers();

    // Deleting unused textures only requires the GL context. This clean-up
    // does not affect screen contents so can happen after swap_buffers...
    texture_cache->drop_unused();

    while (auto const gl_error = glGetError())
        mir::log_debug("GL error: %d", gl_error);
}

void mrg::Renderer::draw(mg::Renderable const& renderable,
                          Renderer::Program const& prog) const
{
    glUseProgram(prog.id);
    if (prog.last_used_frameno != frameno)
    {   // Avoid reloading the screen-global uniforms on every renderable
        prog.last_used_frameno = frameno;
        glUniform1i(prog.tex_uniform, 0);
        glUniformMatrix4fv(prog.display_transform_uniform, 1, GL_FALSE,
                           glm::value_ptr(display_transform));
        glUniformMatrix4fv(prog.screen_to_gl_coords_uniform, 1, GL_FALSE,
                           glm::value_ptr(screen_to_gl_coords));
    }

    glActiveTexture(GL_TEXTURE0);

    auto const& rect = renderable.screen_position();
    GLfloat centrex = rect.top_left.x.as_int() +
                      rect.size.width.as_int() / 2.0f;
    GLfloat centrey = rect.top_left.y.as_int() +
                      rect.size.height.as_int() / 2.0f;
    glUniform2f(prog.centre_uniform, centrex, centrey);

    glUniformMatrix4fv(prog.transform_uniform, 1, GL_FALSE,
                       glm::value_ptr(renderable.transformation()));

    if (prog.alpha_uniform >= 0)
        glUniform1f(prog.alpha_uniform, renderable.alpha());

    glEnableVertexAttribArray(prog.position_attr);
    glEnableVertexAttribArray(prog.texcoord_attr);

    primitives.clear();
    tessellate(primitives, renderable);

    // if we fail to load the texture, we need to carry on (part of lp:1629275)
    try
    {
        auto surface_tex = texture_cache->load(renderable);

        typedef struct  // Represents parameters of glBlendFuncSeparate()
        {
            GLenum src_rgb, dst_rgb, src_alpha, dst_alpha;
        } BlendSeparate;

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

            if (p.tex_id == 0)   // The client surface texture
            {
                blend = client_blend;
                surface_tex->bind();
            }
            else   // Some other texture from the shell (e.g. decorations) which
            {      // is always RGBA (valid SRC_ALPHA).
                blend = {GL_ONE, GL_ONE_MINUS_SRC_ALPHA,
                         GL_ONE, GL_ONE_MINUS_SRC_ALPHA};
                glBindTexture(GL_TEXTURE_2D, p.tex_id);
            }

            glVertexAttribPointer(prog.position_attr, 3, GL_FLOAT,
                                  GL_FALSE, sizeof(mgl::Vertex),
                                  &p.vertices[0].position);
            glVertexAttribPointer(prog.texcoord_attr, 2, GL_FLOAT,
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
        }
    }
    catch (std::exception const& ex)
    {
        report_exception();
    }

    glDisableVertexAttribArray(prog.texcoord_attr);
    glDisableVertexAttribArray(prog.position_attr);
}

void mrg::Renderer::set_viewport(geometry::Rectangle const& rect)
{
    if (rect == viewport)
        return;

    /*
     * Here we provide a 3D perspective projection with a default 30 degrees
     * vertical field of view. This projection matrix is carefully designed
     * such that any vertices at depth z=0 will fit the screen coordinates. So
     * client texels will fit screen pixels perfectly as long as the surface is
     * at depth zero. But if you want to do anything fancy, you can also choose
     * a different depth and it will appear to come out of or go into the
     * screen.
     */
    screen_to_gl_coords = glm::translate(glm::mat4(1.0f), glm::vec3{-1.0f, 1.0f, 0.0f});

    /*
     * Perspective division is one thing that can't be done in a matrix
     * multiplication. It happens after the matrix multiplications. GL just
     * scales {x,y} by 1/w. So modify the final part of the projection matrix
     * to set w ([3]) to be the incoming z coordinate ([2]).
     */
    screen_to_gl_coords[2][3] = -1.0f;

    float const vertical_fov_degrees = 30.0f;
    float const near =
        (rect.size.height.as_int() / 2.0f) /
        std::tan((vertical_fov_degrees * M_PI / 180.0f) / 2.0f);
    float const far = -near;

    screen_to_gl_coords = glm::scale(screen_to_gl_coords,
            glm::vec3{2.0f / rect.size.width.as_int(),
                      -2.0f / rect.size.height.as_int(),
                      2.0f / (near - far)});
    screen_to_gl_coords = glm::translate(screen_to_gl_coords,
            glm::vec3{-rect.top_left.x.as_int(),
                      -rect.top_left.y.as_int(),
                      0.0f});

    viewport = rect;
    update_gl_viewport();
}

void mrg::Renderer::update_gl_viewport()
{
    /*
     * Letterboxing: Move the glViewport to add black bars in the case that
     * the logical viewport aspect ratio doesn't match the display aspect.
     * This keeps pixels square. Note "black"-bars are really glClearColor.
     */
    render_target.ensure_current();

    auto transformed_viewport = display_transform *
                                glm::vec4(viewport.size.width.as_int(),
                                          viewport.size.height.as_int(), 0, 1);
    auto viewport_width = fabs(transformed_viewport[0]);
    auto viewport_height = fabs(transformed_viewport[1]);
    auto dpy = eglGetCurrentDisplay();
    auto surf = eglGetCurrentSurface(EGL_DRAW);
    EGLint buf_width = 0, buf_height = 0;

    if (viewport_width > 0.0f && viewport_height > 0.0f &&
        eglQuerySurface(dpy, surf, EGL_WIDTH, &buf_width) && buf_width > 0 &&
        eglQuerySurface(dpy, surf, EGL_HEIGHT, &buf_height) && buf_height > 0)
    {
        GLint reduced_width = buf_width, reduced_height = buf_height;
        // if viewport_aspect_ratio >= buf_aspect_ratio
        if (viewport_width * buf_height >= buf_width * viewport_height)
            reduced_height = buf_width * viewport_height / viewport_width;
        else
            reduced_width = buf_height * viewport_width / viewport_height;

        GLint offset_x = (buf_width - reduced_width) / 2;
        GLint offset_y = (buf_height - reduced_height) / 2;

        glViewport(offset_x, offset_y, reduced_width, reduced_height);
    }
}

void mrg::Renderer::set_output_transform(glm::mat2 const& t)
{
    auto const new_display_transform = glm::mat4(t);
    if (new_display_transform != display_transform)
    {
        display_transform = new_display_transform;
        update_gl_viewport();
    }
}

void mrg::Renderer::suspend()
{
    texture_cache->invalidate();
}

