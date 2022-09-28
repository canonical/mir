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
 */

#ifndef MIR_RENDERER_GL_BASIC_BUFFER_RENDER_TARGET_H_
#define MIR_RENDERER_GL_BASIC_BUFFER_RENDER_TARGET_H_

#include "buffer_render_target.h"

#include <optional>
#include <GLES2/gl2.h>

namespace mir
{
namespace renderer
{
namespace gl
{
class Context;

/// Not threadsafe, do not use concurrently
class BasicBufferRenderTarget: public BufferRenderTarget
{
public:
    BasicBufferRenderTarget(std::shared_ptr<Context> const& ctx);

    void set_buffer(std::shared_ptr<software::WriteMappableBuffer> const& buffer) override;

    auto size() const -> geometry::Size override;
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
        void copy_to(software::WriteMappableBuffer& buffer);
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

#endif // MIR_RENDERER_GL_BASIC_BUFFER_RENDER_TARGET_H_
