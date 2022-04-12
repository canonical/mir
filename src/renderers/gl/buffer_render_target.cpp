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

#include "mir/renderer/gl/buffer_render_target.h"
#include "mir/renderer/gl/context.h"
#include "mir/renderer/sw/pixel_source.h"
#include "mir/graphics/egl_error.h"

#include <boost/throw_exception.hpp>
#include <GLES2/gl2ext.h>

namespace mg = mir::graphics;
namespace mrg = mir::renderer::gl;
namespace mrs = mir::renderer::software;

mrg::BufferRenderTarget::Framebuffer::Framebuffer(geometry::Size const& size)
    : size{size}
{
    glGenRenderbuffers(1, &colour_buffer);
    glGenFramebuffers(1, &fbo);

    glBindRenderbuffer(GL_RENDERBUFFER, colour_buffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8_OES, size.width.as_int(), size.height.as_int());

    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, colour_buffer);

    auto status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE)
    {
        switch (status)
        {
            case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
                BOOST_THROW_EXCEPTION((
                    std::runtime_error{"FBO is incomplete: GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT"}
                    ));
            case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS:
                // Somehow we've managed to attach buffers with mismatched sizes?
                BOOST_THROW_EXCEPTION((
                    std::logic_error{"FBO is incomplete: GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS"}
                    ));
            case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
                BOOST_THROW_EXCEPTION((
                    std::logic_error{"FBO is incomplete: GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT"}
                    ));
            case GL_FRAMEBUFFER_UNSUPPORTED:
                // This is the only one that isn't necessarily a programming error
                BOOST_THROW_EXCEPTION((
                    std::runtime_error{"FBO is incomplete: formats selected are not supported by this GL driver"}
                    ));
            case 0:
                BOOST_THROW_EXCEPTION((
                    mg::gl_error("Failed to verify GL Framebuffer completeness")));
        }
        BOOST_THROW_EXCEPTION((
            std::runtime_error{
                std::string{"Unknown GL framebuffer error code: "} + std::to_string(status)}));
    }

    glViewport(0, 0, size.width.as_int(), size.height.as_int());
}

mrg::BufferRenderTarget::Framebuffer::~Framebuffer()
{
    glDeleteFramebuffers(1, &fbo);
    glDeleteRenderbuffers(1, &colour_buffer);
}

void mrg::BufferRenderTarget::Framebuffer::copy_to(software::WriteMappableBuffer& buffer)
{
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    auto mapping = buffer.map_writeable();
    if (mapping->size() != size)
    {
        BOOST_THROW_EXCEPTION(std::logic_error("given size does not match buffer size"));
    }
    /*
    * TODO: We are assuming that the framebuffer pixel format is RGBX
    */
    glReadPixels(
        0, 0,
        size.width.as_int(), size.height.as_int(),
        GL_BGRA_EXT, GL_UNSIGNED_BYTE, mapping->data());
}

void mrg::BufferRenderTarget::Framebuffer::bind()
{
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
}

mrg::BufferRenderTarget::BufferRenderTarget(std::shared_ptr<Context> const& ctx)
    : ctx{ctx}
{
}

void mrg::BufferRenderTarget::set_buffer(
    std::shared_ptr<software::WriteMappableBuffer> const& buffer,
    geometry::Size const& size)
{
    this->buffer = buffer;
    if (framebuffer && framebuffer->size == size)
    {
        return;
    }
    framebuffer.reset();
    framebuffer.emplace(size);
}

void mrg::BufferRenderTarget::make_current()
{
    ctx->make_current();
}

void mrg::BufferRenderTarget::release_current()
{
    ctx->release_current();
}

void mrg::BufferRenderTarget::swap_buffers()
{
    if (!framebuffer || !buffer)
    {
        BOOST_THROW_EXCEPTION(std::logic_error("swap_buffers() called when buffer unset"));
    }
    framebuffer->copy_to(*buffer);
}

void mrg::BufferRenderTarget::bind()
{
    if (!framebuffer)
    {
        BOOST_THROW_EXCEPTION(std::logic_error("bind() called without framebuffer"));
    }
    framebuffer->bind();
}
