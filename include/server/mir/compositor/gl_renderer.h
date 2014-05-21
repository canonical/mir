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

#ifndef MIR_COMPOSITOR_GL_RENDERER_H_
#define MIR_COMPOSITOR_GL_RENDERER_H_

#include <mir/compositor/renderer.h>
#include <mir/graphics/gl_program.h>
#include <mir/geometry/rectangle.h>
#include <mir/graphics/buffer_id.h>
#include <mir/graphics/renderable.h>
#include <mir/graphics/gl_primitive.h>
#include <mir/graphics/gl_program_factory.h>
#include <GLES2/gl2.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace mir
{
namespace compositor
{

class GLRenderer : public Renderer
{
public:
    GLRenderer(
        graphics::GLProgramFactory const& program_factory,
        geometry::Rectangle const& display_area);
    virtual ~GLRenderer() noexcept;

    // These are called with a valid GL context:
    void set_viewport(geometry::Rectangle const& rect) override;
    void set_rotation(float degrees) override;
    void begin() const override;
    void render(graphics::RenderableList const&) const override;
    void end() const override;

    // This is called _without_ a GL context:
    void suspend() override;

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
     * \note The cohesion of this function to GLRenderer is quite loose and it
     *       does not strictly need to reside here.
     *       However it seems a good choice under GLRenderer while this remains
     *       the only OpenGL-specific class in the display server, and
     *       tessellation is very much OpenGL-specific.
     */
    virtual void tessellate(std::vector<graphics::GLPrimitive>& primitives,
                            graphics::Renderable const& renderable) const;

    /**
     * Load the texture for a surface any which way you like. The default
     * implementation does so with efficient GPU-side caching built in.
     *
     * \returns The OpenGL texture name for the surface.
     */
    virtual GLuint load_texture(graphics::Renderable const& renderable,
                                graphics::Buffer& buffer) const;

    virtual void render(graphics::Renderable const& renderable) const;

private:
    std::unique_ptr<graphics::GLProgram> program;
    GLuint position_attr_loc;
    GLuint texcoord_attr_loc;
    GLuint centre_uniform_loc;
    GLuint display_transform_uniform_loc;
    GLuint transform_uniform_loc;
    GLuint alpha_uniform_loc;
    float rotation;

    geometry::Rectangle viewport;

    struct Texture
    {
        GLuint id = 0;
        graphics::BufferID origin;
        bool used;
    };
    mutable std::unordered_map<graphics::Renderable::ID, Texture> textures;
    mutable std::unordered_set<std::shared_ptr<graphics::Buffer>> saved_resources;
    mutable bool skipped = false;

};

}
}

#endif // MIR_COMPOSITOR_GL_RENDERER_H_
