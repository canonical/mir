/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_RENDERER_GL_RENDERER_H_
#define MIR_RENDERER_GL_RENDERER_H_

#include "program_family.h"

#include <mir/compositor/renderer.h>
#include <mir/geometry/rectangle.h>
#include <mir/graphics/buffer_id.h>
#include <mir/graphics/renderable.h>
#include <mir/gl/primitive.h>
#include "mir/renderer/gl/render_target.h"

#include <GLES2/gl2.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace mir
{
namespace gl { class TextureCache; }
namespace graphics { class DisplayBuffer; }
namespace renderer
{
namespace gl
{

class CurrentRenderTarget
{
public:
    CurrentRenderTarget(graphics::DisplayBuffer* display_buffer);
    ~CurrentRenderTarget();

    void ensure_current();
    void swap_buffers();

private:
    renderer::gl::RenderTarget* const render_target;
};

class Renderer : public compositor::Renderer
{
public:
    Renderer(graphics::DisplayBuffer& display_buffer);
    virtual ~Renderer();

    // These are called with a valid GL context:
    void set_viewport(geometry::Rectangle const& rect) override;
    void set_rotation(float degrees) override;
    void render(graphics::RenderableList const&) const override;

    // This is called _without_ a GL context:
    void suspend() override;

private:
    mutable CurrentRenderTarget render_target;

protected:
    /**
     * tessellate defines the list of triangles that will be used to render
     * the surface. By default it just returns 4 vertices for a rectangle.
     * However you can override its behaviour to tessellate more finely and
     * deform freely for effects like wobbly windows.
     *
     * \param [in,out] primitives The list of rendering primitives to be
     *                            grown and/or modified.
     * \param [in]     renderable The renderable surface being tessellated.
     *
     * \note The cohesion of this function to gl::Renderer is quite loose and it
     *       does not strictly need to reside here.
     *       However it seems a good choice under gl::Renderer while this remains
     *       the only OpenGL-specific class in the display server, and
     *       tessellation is very much OpenGL-specific.
     */
    virtual void tessellate(std::vector<mir::gl::Primitive>& primitives,
                            graphics::Renderable const& renderable) const;

    GLfloat clear_color[4];

    mutable long long frameno = 0;

    ProgramFamily family;
    struct Program
    {
       GLuint id = 0;
       GLint tex_uniform = -1;
       GLint position_attr = -1;
       GLint texcoord_attr = -1;
       GLint centre_uniform = -1;
       GLint display_transform_uniform = -1;
       GLint transform_uniform = -1;
       GLint screen_to_gl_coords_uniform = -1;
       GLint alpha_uniform = -1;
       mutable long long last_used_frameno = 0;

       Program(GLuint program_id);
    };
    Program default_program, alpha_program;

    static const GLchar* const vshader;
    static const GLchar* const default_fshader;
    static const GLchar* const alpha_fshader;

    virtual void draw(graphics::Renderable const& renderable,
                      Renderer::Program const& prog) const;

private:
    std::unique_ptr<mir::gl::TextureCache> const texture_cache;
    float rotation;
    geometry::Rectangle viewport;
    glm::mat4 screen_to_gl_coords, screen_rotation;

    std::vector<mir::gl::Primitive> mutable primitives;
};

}
}
}

#endif // MIR_RENDERER_GL_RENDERER_H_
