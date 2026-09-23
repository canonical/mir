/* Copyright © Canonical Ltd.
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
 */

#define MIR_LOG_COMPONENT "GLRenderer"

#include <mir/renderers/gl/renderer.h>
#include <mir/graphics/renderable.h>
#include <mir/graphics/display_sink.h>
#include <mir/log.h>
#include <mir/graphics/egl_error.h>
#include <mir/graphics/rendering_providers.h>
#include <mir/graphics/texture.h>
#include <mir/renderer/gl/gl_surface.h>

#include "common/gl_output_filter.h"
#include "common/gl_program_factory.h"
#include "common/gl_scene_drawing.h"

#include <EGL/egl.h>

#include <boost/throw_exception.hpp>
#include <ranges>

namespace mg = mir::graphics;
namespace mrc = mir::renderer::common;
namespace mrg = mir::renderer::gl;

class mrg::Renderer::ProgramFactory : public mrc::GLProgramFactory
{
};

class mrg::Renderer::OutputFilter : public mg::gl::OutputSurface
{
public:
    // NOTE: This must be called with a current GL context
    OutputFilter(std::unique_ptr<mg::gl::OutputSurface> output)
     : output{std::move(output)}
    {
    }

    void set_filter(MirOutputFilter filter)
    {
        this->filter.set_filter(filter);
    }

    void bind() override
    {
        if (!filter.bind_intermediate(output->size()))
            output->bind();
    }

    void make_current() override
    {
        output->make_current();
    }

    void release_current() override
    {
        output->release_current();
    }

    auto commit() -> std::unique_ptr<mg::Framebuffer> override
    {
        if (filter.active())
        {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            output->bind();
            filter.apply();
        }

        return output->commit();
    }

    auto size() const -> mir::geometry::Size override
    {
        return output->size();
    }

    auto layout() const -> Layout override
    {
        return output->layout();
    }

private:
    std::unique_ptr<mg::gl::OutputSurface> output;
    mrc::GLOutputFilter filter;
};

mrg::Renderer::Program::Program(GLuint program_id)
{
    id = program_id;
    position_attr = glGetAttribLocation(id, "position");
    texcoord_attr = glGetAttribLocation(id, "texcoord");
    for (auto const [index, uniform] : tex_uniforms | std::views::enumerate)
    {
        /* You can reference uniform arrays as tex[0], tex[1], tex[2], … until you
         * hit the end of the array, which will return -1 as the location.
         */
        auto const uniform_name = std::string{"tex["} + std::to_string(index) + "]";
        uniform = glGetUniformLocation(id, uniform_name.c_str());
    }
    centre_uniform = glGetUniformLocation(id, "centre");
    oriented_centre = glGetUniformLocation(id, "oriented_centre");
    display_transform_uniform = glGetUniformLocation(id, "display_transform");
    orientation_transform_uniform = glGetUniformLocation(id, "orientation_transform");
    transform_uniform = glGetUniformLocation(id, "transform");
    screen_to_gl_coords_uniform = glGetUniformLocation(id, "screen_to_gl_coords");
    alpha_uniform = glGetUniformLocation(id, "alpha");
}

namespace
{
auto make_output_current(std::unique_ptr<mg::gl::OutputSurface> output) -> std::unique_ptr<mg::gl::OutputSurface>
{
    output->make_current();
    return output;
}
}

mrg::Renderer::Renderer(
    std::shared_ptr<graphics::GLRenderingProvider> gl_interface,
    std::unique_ptr<graphics::gl::OutputSurface> output)
    : output_surface{std::make_unique<OutputFilter>(make_output_current(std::move(output)))},
      clear_color{0.0f, 0.0f, 0.0f, 1.0f},
      program_factory{std::make_unique<ProgramFactory>()},
      screen_to_gl_coords(0),
      display_transform(1),
      gl_interface{std::move(gl_interface)}
{
    eglBindAPI(EGL_OPENGL_ES_API);
    EGLDisplay disp = eglGetCurrentDisplay();
    if (disp != EGL_NO_DISPLAY)
    {
        struct {GLint id; char const* label;} const eglstrings[] =
        {
            {EGL_VENDOR,      "EGL vendor"},
            {EGL_VERSION,     "EGL version"},
            {EGL_CLIENT_APIS, "EGL client APIs"},
            {EGL_EXTENSIONS,  "EGL extensions"},
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
        {GL_EXTENSIONS, "GL extensions"},
    };

    for (auto& s : glstrings)
    {
        auto val = reinterpret_cast<char const*>(glGetString(s.id)); //TICS !cppcoreguidelines-pro-type-reinterpret-cast: glGetString returns an ASCII string, guaranteed not to have the high-bit set, so it's representationally-identical to signed char
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
}

mrg::Renderer::~Renderer()
{
    auto prev_dpy = eglGetCurrentDisplay();
    auto prev_ctx = eglGetCurrentContext();
    auto prev_read = eglGetCurrentSurface(EGL_READ);
    auto prev_draw = eglGetCurrentSurface(EGL_DRAW);

    // We are going to be releasing GL resources; that means we have to have a current context
    // We've allocated all our resources on the context of output_surface; release them there, too.
    output_surface->make_current();

    auto const output_surf_ctx = eglGetCurrentContext();

    program_factory.reset();

    // OutputSurface destructor correctly cleans up, leaving no EGL context current
    output_surface.reset();

    // If our output surface was not previously current, restore EGL state
    // (If the output surface *was* current, then it's not a good idea to try and restore that state!)
    if (prev_ctx != output_surf_ctx)
    {
        eglMakeCurrent(prev_dpy, prev_draw, prev_read, prev_ctx);
    }
}

auto mrg::Renderer::render(mg::RenderableList const& renderables) const -> std::unique_ptr<mg::Framebuffer>
{
    output_surface->make_current();
    output_surface->bind();

    mrc::clear(clear_color);

    ++frameno;
    for (auto const& r : renderables)
    {
        draw(*r);
    }

    auto output = output_surface->commit();

    // Report any GL errors after commit, to catch any *during* commit
    while (auto const gl_error = glGetError())
        mir::log_debug("GL error: %d", gl_error);

    return output;
}

void mrg::Renderer::draw(mg::Renderable const& renderable) const
{
    auto const texture = gl_interface->as_texture(renderable.buffer());
    primitives.clear();
    tessellate(primitives, renderable);

    mrc::draw(
        mrc::SceneContext{
            .viewport = viewport,
            .output_size = output_surface->size(),
            .display_transform = display_transform,
            .screen_to_gl_coords = screen_to_gl_coords,
            .frameno = frameno},
        renderable,
        *texture,
        *program_factory,
        primitives);
}

void mrg::Renderer::tessellate(std::vector<mir::gl::Primitive>& primitives, mg::Renderable const& renderable) const
{
    mrc::tessellate(primitives, renderable);
}

void mrg::Renderer::set_viewport(geometry::Rectangle const& rect)
{
    /* The output surface can change size under us (the screen shooter captures
     * to whatever buffer the client hands us), which changes the GL viewport
     * even when the logical area is unchanged.
     */
    if (rect == viewport)
    {
        if (output_surface->size() != last_output_size)
            update_gl_viewport();
        return;
    }

    screen_to_gl_coords = mrc::screen_to_gl_coords_for(rect);

    viewport = rect;
    update_gl_viewport();
}

void mrg::Renderer::update_gl_viewport()
{
    output_surface->make_current();
    output_surface->bind();

    last_output_size = output_surface->size();
    mrc::set_letterboxed_gl_viewport(viewport, display_transform, last_output_size);
}

void mrg::Renderer::set_output_transform(glm::mat2 const& t)
{
    auto new_display_transform = glm::mat4(t);

    switch (output_surface->layout())
    {
    case graphics::gl::OutputSurface::Layout::GL:
        break;
    case graphics::gl::OutputSurface::Layout::TopRowFirst:
        // GL is going to render in its own coordinate system, but the OutputSurface
        // wants the output to be the other way up. Get GL to render upside-down instead.
        new_display_transform = glm::mat4{
            1.0, 0.0, 0.0, 0.0,
            0.0, -1.0, 0.0, 0.0,
            0.0, 0.0, 1.0, 0.0,
            0.0, 0.0, 0.0, 1.0
        } * new_display_transform;
        break;
    }

    if (new_display_transform != display_transform)
    {
        display_transform = new_display_transform;
        update_gl_viewport();
    }
}

void mrg::Renderer::set_output_filter(MirOutputFilter filter)
{
    output_surface->set_filter(filter);
}

void mrg::Renderer::suspend()
{
    output_surface->release_current();
}
