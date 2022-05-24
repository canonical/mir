/*
 * Copyright 2021 UBports Foundation.
 * Copyright © 2017-2020 Canonical Ltd.
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
 */

#include "miroil/mirbuffer.h"

#include <mir/graphics/buffer.h>
#include <mir/graphics/texture.h>

#if MIR_SERVER_VERSION < MIR_VERSION_NUMBER(2, 3, 0)
#include <mir/gl/texture.h>
#include <mir/renderer/gl/texture_source.h>
#endif

#include <stdexcept>

miroil::GLBuffer::~GLBuffer() = default;

miroil::GLBuffer::GLBuffer(std::shared_ptr<mir::graphics::Buffer> const& buffer) :
    wrapped(buffer)
{
}

std::shared_ptr<miroil::GLBuffer> miroil::GLBuffer::from_mir_buffer(std::shared_ptr<mir::graphics::Buffer> const& buffer)
{
#if MIR_SERVER_VERSION < MIR_VERSION_NUMBER(2, 3, 0)
    bool usingTextureSource = false;

    // We would like to use gl::Texture, but if we cant, fallback to gl::textureSource
    if (!dynamic_cast<mir::graphics::gl::Texture*>(buffer->native_buffer_base()))
        usingTextureSource = true;

    qDebug() << "Mir buffer is" << (usingTextureSource ? "gl:TextureSource (old)" : "gl:Texture (new)");

    if (usingTextureSource)
        return std::make_shared<miroil::GLTextureSourceBuffer>(buffer);
    else
#endif
        return std::make_shared<miroil::GLTextureBuffer>(buffer);
}

void miroil::GLBuffer::setWrapped(std::shared_ptr<mir::graphics::Buffer> const& buffer)
{
    wrapped = buffer;
}

void miroil::GLBuffer::reset()
{
    wrapped.reset();
}

bool miroil::GLBuffer::empty()
{
    return !wrapped;
}

bool miroil::GLBuffer::has_alpha_channel() const
{
    return wrapped &&
        (wrapped->pixel_format() == mir_pixel_format_abgr_8888
        || wrapped->pixel_format() == mir_pixel_format_argb_8888);
}

mir::geometry::Size miroil::GLBuffer::size() const
{
    return wrapped->size();
}

#if MIR_SERVER_VERSION < MIR_VERSION_NUMBER(2, 3, 0)
miroil::GLTextureSourceBuffer::GLTextureSourceBuffer(std::shared_ptr<mir::graphics::Buffer> const& buffer) :
    GLBuffer(buffer)
{
    auto texsource = dynamic_cast<mir::renderer::gl::TextureSource*>(buffer->native_buffer_base());
    if (!texsource)
        throw std::runtime_error("Incompatible buffer for GLTextureSourceBuffer");
    m_texSource = texsource;
}

void miroil::GLTextureSourceBuffer::setWrapped(const std::shared_ptr<mir::graphics::Buffer> &buffer)
{
    auto texsource = dynamic_cast<mir::renderer::gl::TextureSource*>(buffer->native_buffer_base());
    if (!texsource)
        throw std::runtime_error("Incompatible buffer for GLTextureSourceBuffer");

    GLBuffer::setWrapped(buffer);

    m_texSource = texsource;
}

void miroil::GLTextureSourceBuffer::upload_to_texture()
{
    if (!wrapped)
        throw std::logic_error("Bind called without any buffers!");

    if (m_texSource) {
        m_texSource->gl_bind_to_texture();
        m_texSource->secure_for_render();
    } else {
        throw std::logic_error("Buffer does not support GL rendering");
    }
}
#endif

miroil::GLTextureBuffer::GLTextureBuffer(std::shared_ptr<mir::graphics::Buffer> const& buffer) :
    GLBuffer(buffer)
{
    auto texture = dynamic_cast<mir::graphics::gl::Texture*>(buffer->native_buffer_base());
    if (!texture)
        throw std::runtime_error("Incompatible buffer for GLTextureBuffer");
    m_mirTex = texture;
}

void miroil::GLTextureBuffer::setWrapped(const std::shared_ptr<mir::graphics::Buffer> &buffer)
{
    auto texture = dynamic_cast<mir::graphics::gl::Texture*>(buffer->native_buffer_base());
    if (!texture)
        throw std::runtime_error("Incompatible buffer for GLTextureBuffer");

    GLBuffer::setWrapped(buffer);

    m_mirTex = texture;
}

void miroil::GLTextureBuffer::tex_bind()
{
    if (!wrapped)
        throw std::logic_error("Bind called without any buffers!");

    if (auto const texture = dynamic_cast<mir::graphics::gl::Texture*>(wrapped->native_buffer_base())) {
        texture->bind();
    } else {
        throw std::logic_error("Buffer does not support GL rendering");
    }
}
