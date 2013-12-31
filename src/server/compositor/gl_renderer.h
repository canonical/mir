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

#include "renderer.h"
#include "mir/geometry/rectangle.h"
#include <GLES2/gl2.h>

namespace mir
{
namespace compositor
{

class GLRenderer : public Renderer
{
public:
    GLRenderer(geometry::Rectangle const& display_area);
    virtual ~GLRenderer() noexcept;

    void begin() const override;
    void render(CompositingCriteria const& info, graphics::Buffer& buffer) const override;
    void end() const override;

private:
    GLuint vertex_shader;
    GLuint fragment_shader;
    GLuint program;
    GLuint position_attr_loc;
    GLuint texcoord_attr_loc;
    GLuint transform_uniform_loc;
    GLuint alpha_uniform_loc;
    GLuint vertex_attribs_vbo;
    GLuint texture;
};

}
}

#endif // MIR_COMPOSITOR_GL_RENDERER_H_
