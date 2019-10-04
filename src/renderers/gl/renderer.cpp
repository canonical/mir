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
#include "mir/graphics/egl_error.h"
#include "mir/graphics/texture.h"
#include "mir/graphics/program_factory.h"
#include "mir/graphics/program.h"

#define GLM_FORCE_RADIANS
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <EGL/egl.h>

#include <boost/throw_exception.hpp>
#include <stdexcept>
#include <cmath>
#include <sstream>

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

namespace
{
template<void (* deleter)(GLuint)>
class GLHandle
{
public:
    explicit GLHandle(GLuint id)
        : id{id}
    {
    }

    ~GLHandle()
    {
        if (id)
            (*deleter)(id);
    }

    GLHandle(GLHandle const&) = delete;

    GLHandle& operator=(GLHandle const&) = delete;

    GLHandle(GLHandle&& from)
        : id{from.id}
    {
        from.id = 0;
    }

    operator GLuint() const
    {
        return id;
    }

private:
    GLuint id;
};

using ProgramHandle = GLHandle<&glDeleteProgram>;
using ShaderHandle = GLHandle<&glDeleteShader>;

struct Program : public mir::graphics::gl::Program
{
public:
    Program(ProgramHandle&& opaque_shader, ProgramHandle&& alpha_shader)
        : opaque_handle(std::move(opaque_shader)),
          alpha_handle(std::move(alpha_shader)),
          opaque{opaque_handle},
          alpha{alpha_handle}
    {
    }

    ProgramHandle opaque_handle, alpha_handle;
    mir::renderer::gl::Renderer::Program opaque, alpha;
};

const GLchar* const vertex_shader_src =
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
}

class mrg::Renderer::ProgramFactory : public mir::graphics::gl::ProgramFactory
{
public:
    // NOTE: This must be called with a current GL context
    ProgramFactory()
        : vertex_shader{compile_shader(GL_VERTEX_SHADER, vertex_shader_src)}
    {
    }

    std::unique_ptr<mir::graphics::gl::Program>
    compile_fragment_shader(
        char const* extension_fragment,
        char const* fragment_fragment) override
    {
        std::stringstream opaque_fragment;
        opaque_fragment
            <<
            "#ifdef GL_ES\n"
            "precision mediump float;\n"
            "#endif\n"
            << "\n"
            << extension_fragment
            << "\n"
            << fragment_fragment
            << "\n"
            <<
            "varying vec2 v_texcoord;\n"
            "void main() {\n"
            "    gl_FragColor = sample_to_rgba(v_texcoord);\n"
            "}\n";

        std::stringstream alpha_fragment;
        alpha_fragment
            <<
            "#ifdef GL_ES\n"
            "precision mediump float;\n"
            "#endif\n"
            << "\n"
            << extension_fragment
            << "\n"
            << fragment_fragment
            << "\n"
            <<
            "varying vec2 v_texcoord;\n"
            "uniform float alpha;\n"
            "void main() {\n"
            "    gl_FragColor = alpha * sample_to_rgba(v_texcoord);\n"
            "}\n";

        // GL shader compilation is *not* threadsafe, and requires external synchronisation
        std::lock_guard<std::mutex> lock{compilation_mutex};

        ShaderHandle const opaque_shader{
            compile_shader(GL_FRAGMENT_SHADER, opaque_fragment.str().c_str())};
        ShaderHandle const alpha_shader{
            compile_shader(GL_FRAGMENT_SHADER, alpha_fragment.str().c_str())};

        return std::make_unique<::Program>(
            link_shader(vertex_shader, opaque_shader),
            link_shader(vertex_shader, alpha_shader));

        // We delete opaque_shader and alpha_shader here. This is fine; it only marks them
        // for deletion. GL will only delete them once the GL Program they're linked in is destroyed.
    }

private:
    static GLuint compile_shader(GLenum type, GLchar const* src)
    {
        GLuint id = glCreateShader(type);
        if (!id)
        {
            BOOST_THROW_EXCEPTION(mg::gl_error("Failed to create shader"));
        }

        glShaderSource(id, 1, &src, NULL);
        glCompileShader(id);
        GLint ok;
        glGetShaderiv(id, GL_COMPILE_STATUS, &ok);
        if (!ok)
        {
            GLchar log[1024];
            glGetShaderInfoLog(id, sizeof log - 1, NULL, log);
            log[sizeof log - 1] = '\0';
            glDeleteShader(id);
            BOOST_THROW_EXCEPTION(
                std::runtime_error(
                    std::string("Compile failed: ") + log + " for:\n" + src));
        }
        return id;
    }

    static ProgramHandle link_shader(
        ShaderHandle const& vertex_shader,
        ShaderHandle const& fragment_shader)
    {
        ProgramHandle program{glCreateProgram()};
        glAttachShader(program, fragment_shader);
        glAttachShader(program, vertex_shader);
        glLinkProgram(program);
        GLint ok;
        glGetProgramiv(program, GL_LINK_STATUS, &ok);
        if (!ok)
        {
            GLchar log[1024];
            glGetProgramInfoLog(program, sizeof log - 1, NULL, log);
            log[sizeof log - 1] = '\0';
            BOOST_THROW_EXCEPTION(
                std::runtime_error(
                    std::string("Linking GL shader failed: ") + log));
        }

        return program;
    }

    ShaderHandle const vertex_shader;
    // GL requires us to synchronise multi-threaded access to the shader APIs.
    std::mutex compilation_mutex;
};

mrg::Renderer::Program::Program(GLuint program_id)
{
    id = program_id;
    position_attr = glGetAttribLocation(id, "position");
    texcoord_attr = glGetAttribLocation(id, "texcoord");
    for (auto i = 0u; i < tex_uniforms.size() ; ++i)
    {
        /* You can reference uniform arrays as tex[0], tex[1], tex[2], … until you
         * hit the end of the array, which will return -1 as the location.
         */
        auto const uniform_name = std::string{"tex["} + std::to_string(i) + "]";
        tex_uniforms[i] = glGetUniformLocation(id, uniform_name.c_str());
    }
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
      program_factory{std::make_unique<ProgramFactory>()},
      texture_cache(mgl::DefaultProgramFactory().create_texture_cache()),
      display_transform(1)
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
    {
        draw(*r);
    }

    render_target.swap_buffers();

    // Deleting unused textures only requires the GL context. This clean-up
    // does not affect screen contents so can happen after swap_buffers...
    texture_cache->drop_unused();

    while (auto const gl_error = glGetError())
        mir::log_debug("GL error: %d", gl_error);
}

void mrg::Renderer::draw(mg::Renderable const& renderable) const
{
    auto const clip_area = renderable.clip_area();
    if (clip_area)
    {
        glEnable(GL_SCISSOR_TEST);
        glScissor(
            clip_area.value().top_left.x.as_int() -
                viewport.top_left.x.as_int(),
            viewport.top_left.y.as_int() +
                viewport.size.height.as_int() -
                clip_area.value().top_left.y.as_int() -
                clip_area.value().size.height.as_int(),
            clip_area.value().size.width.as_int(), 
            clip_area.value().size.height.as_int()
        );
    }

    auto const texture = std::dynamic_pointer_cast<mg::gl::Texture>(renderable.buffer());
    auto const surface_tex =
        [this, &renderable, need_fallback = !static_cast<bool>(texture)]() -> std::shared_ptr<mir::gl::Texture>
        {
            if (need_fallback)
            {
                try
                {
                    return texture_cache->load(renderable);
                }
                catch (std::exception const&)
                {
                    report_exception();
                }
            }
            return {nullptr};
        }();

    auto const* maybe_prog =
        [this, &texture, &surface_tex](bool alpha) -> Program const*
        {
            if (texture)
            {
                auto const& family = static_cast<::Program const&>(texture->shader(*program_factory));
                if (alpha)
                {
                    return &family.alpha;
                }
                return &family.opaque;
            }
            else if(surface_tex)
            {
                if (alpha)
                {
                    return &alpha_program;
                }
                return &default_program;
            }
            return nullptr;
        }(renderable.alpha() < 1.0f);

    if (!maybe_prog)
    {
        mir::log_error("Buffer does not support GL rendering!");
        return;
    }

    auto const& prog = *maybe_prog;

    glUseProgram(prog.id);
    if (prog.last_used_frameno != frameno)
    {   // Avoid reloading the screen-global uniforms on every renderable
        // TODO: We actually only need to bind these *once*, right? Not once per frame?
        prog.last_used_frameno = frameno;
        for (auto i = 0u; i < prog.tex_uniforms.size(); ++i)
        {
            if (prog.tex_uniforms[i] != -1)
            {
                glUniform1i(prog.tex_uniforms[i], i);
            }
        }
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

    glm::mat4 transform = renderable.transformation();
    if (texture && (texture->layout() == mg::gl::Texture::Layout::TopRowFirst))
    {
        // GL textures have (0,0) at bottom-left rather than top-left
        // We have to invert this texture to get it the way up GL expects.
        transform *= glm::mat4{
            1.0, 0.0, 0.0, 0.0,
            0.0, -1.0, 0.0, 0.0,
            0.0, 0.0, 1.0, 0.0,
            -1.0, 1.0, 0.0, 1.0
        };
    }

    glUniformMatrix4fv(prog.transform_uniform, 1, GL_FALSE,
                       glm::value_ptr(transform));

    if (prog.alpha_uniform >= 0)
        glUniform1f(prog.alpha_uniform, renderable.alpha());

    glEnableVertexAttribArray(prog.position_attr);
    glEnableVertexAttribArray(prog.texcoord_attr);

    primitives.clear();
    tessellate(primitives, renderable);

    // if we fail to load the texture, we need to carry on (part of lp:1629275)
    try
    {
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

            blend = client_blend;
            if (surface_tex)
            {
                surface_tex->bind();
            }
            else
            {
                texture->bind();
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

            if (texture)
            {
                // We're done with the texture for now
                texture->add_syncpoint();
            }
        }
    }
    catch (std::exception const& ex)
    {
        report_exception();
    }

    glDisableVertexAttribArray(prog.texcoord_attr);
    glDisableVertexAttribArray(prog.position_attr);
    if (renderable.clip_area())
    {
        glDisable(GL_SCISSOR_TEST);
    }
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

