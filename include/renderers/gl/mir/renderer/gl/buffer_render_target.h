/*
 * Copyright © 2022 Canonical Ltd.
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
 * Authored By: William Wold <william.wold@canonical.com>
 */

#ifndef MIR_RENDERER_GL_BUFFER_RENDER_TARGET_H_
#define MIR_RENDERER_GL_BUFFER_RENDER_TARGET_H_

#include "render_target.h"
#include "mir/geometry/size.h"

#include <memory>
#include <optional>
#include <GLES2/gl2.h>

namespace mir
{
namespace renderer
{
namespace software
{
class WriteMappableBuffer;
}
namespace gl
{
class Context;

template<void (*allocator)(GLsizei, GLuint*), void (* deleter)(GLsizei, GLuint const*)>
class GLHandle
{
public:
    GLHandle()
    {
        (*allocator)(1, &id);
    }

    ~GLHandle()
    {
        if (id)
            (*deleter)(1, &id);
    }

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

using RenderbufferHandle = GLHandle<&glGenRenderbuffers, &glDeleteRenderbuffers>;
using FramebufferHandle = GLHandle<&glGenFramebuffers, &glDeleteFramebuffers>;

/// Not threadsafe, do not use concurrently
class BufferRenderTarget: public RenderTarget
{
public:
    BufferRenderTarget(std::shared_ptr<Context> const& ctx);

    void set_buffer(
        std::shared_ptr<software::WriteMappableBuffer> const& buffer,
        geometry::Size const& size);

    void make_current() override;
    void release_current() override;
    void swap_buffers() override;
    void bind() override;

private:
    class Framebuffer
    {
    public:
        Framebuffer(geometry::Size const& size);
        ~Framebuffer();
        void render(software::WriteMappableBuffer& buffer);
        void bind();

        geometry::Size const size;

    private:
        Framebuffer(Framebuffer const&) = delete;
        Framebuffer& operator=(Framebuffer const&) = delete;

        GLuint colour_buffer;
        GLuint fbo;
    };

    std::shared_ptr<Context> const ctx;

    std::shared_ptr<software::WriteMappableBuffer> buffer{nullptr};
    std::optional<Framebuffer> framebuffer;
};

}
}
}

#endif // MIR_RENDERER_GL_BUFFER_RENDER_TARGET_H_
