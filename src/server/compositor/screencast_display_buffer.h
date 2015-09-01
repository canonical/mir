/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_COMPOSITOR_SCREENCAST_DISPLAY_BUFFER_H_
#define MIR_COMPOSITOR_SCREENCAST_DISPLAY_BUFFER_H_

#include "mir/graphics/display_buffer.h"
#include "mir/renderer/gl/texture_bindable.h"

#include <GLES2/gl2.h>

namespace mir
{
namespace compositor
{
namespace detail
{

template <void (*Generate)(GLsizei,GLuint*), void (*Delete)(GLsizei,GLuint const*)>
class GLResource
{
public:
    GLResource() { Generate(1, &resource); }
    ~GLResource() { Delete(1, &resource); }
    operator GLuint() const { return resource; }

private:
    GLResource(GLResource const&) = delete;
    GLResource& operator=(GLResource const&) = delete;
    GLuint resource = 0;
};
}

class ScreencastDisplayBuffer : public graphics::DisplayBuffer
{
public:
    ScreencastDisplayBuffer(
        geometry::Rectangle const& rect,
        graphics::Buffer& buffer);
    ~ScreencastDisplayBuffer();

    geometry::Rectangle view_area() const override;

    void make_current() override;

    void release_current() override;

    bool post_renderables_if_optimizable(graphics::RenderableList const&) override;

    void gl_swap_buffers() override;

    MirOrientation orientation() const override;

private:
    geometry::Rectangle const rect;
    graphics::Buffer& buffer;
    renderer::gl::TextureBindable* texture_bindable;
    GLint old_fbo;
    GLint old_viewport[4];
    detail::GLResource<glGenTextures,glDeleteTextures> const color_tex;
    detail::GLResource<glGenRenderbuffers,glDeleteRenderbuffers> const depth_rbo;
    detail::GLResource<glGenFramebuffers,glDeleteFramebuffers> const fbo;
};

}
}

#endif /* MIR_COMPOSITOR_SCREENCAST_DISPLAY_BUFFER_H_ */
