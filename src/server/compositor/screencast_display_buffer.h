/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_COMPOSITOR_SCREENCAST_DISPLAY_BUFFER_H_
#define MIR_COMPOSITOR_SCREENCAST_DISPLAY_BUFFER_H_

#include "mir/graphics/display_buffer.h"
#include "mir/renderer/gl/render_target.h"

#include MIR_SERVER_GL_H
#include MIR_SERVER_GLEXT_H

namespace mir
{
namespace graphics
{
class Display;
}

namespace renderer
{
namespace gl
{
class Context;
class TextureSource;
}
}

namespace compositor
{

namespace detail
{

template <void (*Delete)(GLsizei,GLuint const*)>
class GLResource
{
public:
    GLResource() {}
    GLResource(GLuint resource) : resource{resource} {}
    ~GLResource() { reset(); }

    operator GLuint() const { return resource; }

    void reset()
    {
        if (resource)
        {
            Delete(1, &resource);
            resource = 0;
        }
    }

    GLResource(GLResource&& other) : resource {other.resource}
    {
        other.resource = 0;
    }

    GLResource& operator=(GLResource&& other)
    {
        resource = other.resource;
        other.resource = 0;
        return *this;
    }

private:
    GLResource(GLResource const&) = delete;
    GLResource& operator=(GLResource const&) = delete;
    GLuint resource{0};
};
}

class Schedule;
class ScreencastDisplayBuffer : public graphics::DisplayBuffer,
                                public graphics::NativeDisplayBuffer,
                                public renderer::gl::RenderTarget
{
public:
    ScreencastDisplayBuffer(
        geometry::Rectangle const& rect,
        geometry::Size const& size,
        MirMirrorMode mirror_mode,
        Schedule& free_queue,
        Schedule& ready_queue,
        graphics::Display& display);
    ~ScreencastDisplayBuffer();

    geometry::Rectangle view_area() const override;

    void make_current() override;

    void bind() override;

    void release_current() override;

    bool overlay(graphics::RenderableList const&) override;

    void swap_buffers() override;

    glm::mat2 transformation() const override;

    NativeDisplayBuffer* native_display_buffer() override;

    geometry::Size renderbuffer_size();
    void set_renderbuffer_size(geometry::Size);
    void set_transformation(glm::mat2 const& transform);
    void commit();

private:
    std::unique_ptr<renderer::gl::Context> gl_context;
    geometry::Rectangle const rect;
    glm::mat2 transform;

    Schedule& free_queue;
    Schedule& ready_queue;
    std::shared_ptr<graphics::Buffer> current_buffer;

    GLint old_fbo;
    GLint old_viewport[4];

    detail::GLResource<glDeleteTextures> color_tex;
    detail::GLResource<glDeleteRenderbuffers> depth_rbo;
    detail::GLResource<glDeleteFramebuffers> fbo;

    geometry::Size current_size;
};

}
}

#endif /* MIR_COMPOSITOR_SCREENCAST_DISPLAY_BUFFER_H_ */
