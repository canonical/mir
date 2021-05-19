/*
 * Copyright 2021 UBports Foundation.
 * Copyright © 2017 Canonical Ltd.
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
#include "mir/graphics/texture.h"
#include <mir/renderer/gl/texture_source.h>

#include <stdexcept>

miroil::GLBuffer::~GLBuffer() = default;

miroil::GLBuffer::GLBuffer(std::shared_ptr<mir::graphics::Buffer> const& buffer) :
    wrapped(buffer)
{
}

std::shared_ptr<miroil::GLBuffer> miroil::GLBuffer::from_mir_buffer(std::shared_ptr<mir::graphics::Buffer> const& buffer)
{
    bool usingTextureSource = false;

    // We would like to use gl::Texture, but if we cant, fallback to gl::textureSource
    if (!dynamic_cast<mir::graphics::gl::Texture*>(buffer->native_buffer_base()))
        // As textures will never change once inited, there is no need to do vodo magic
        // on each bind(), so lets create class overrides to save some cpu cycles.
        usingTextureSource = true;

    //qDebug() << "Mir buffer is" << (usingTextureSource ? "gl:TextureSource (old)" : "gl:Texture (new)");

    if (usingTextureSource)
        return std::make_shared<miroil::GLTextureSourceBuffer>(buffer);
    else
        return std::make_shared<miroil::GLTextureBuffer>(buffer);
}

void miroil::GLBuffer::reset(std::shared_ptr<mir::graphics::Buffer> const& buffer)
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

miroil::GLTextureSourceBuffer::GLTextureSourceBuffer(std::shared_ptr<mir::graphics::Buffer> const& buffer) :
    GLBuffer(buffer)
{
}

void miroil::GLTextureSourceBuffer::bind()
{
    if (!wrapped)
        throw std::logic_error("Bind called without any buffers!");

    if (auto const texsource = dynamic_cast<mir::renderer::gl::TextureSource*>(wrapped->native_buffer_base())) {
        texsource->gl_bind_to_texture();
        texsource->secure_for_render();
    } else {
        throw std::logic_error("Buffer does not support GL rendering");
    }
}

miroil::GLTextureBuffer::GLTextureBuffer(std::shared_ptr<mir::graphics::Buffer> const& buffer) :
    GLBuffer(buffer)
{
}

void miroil::GLTextureBuffer::bind()
{
    if (!wrapped)
        throw std::logic_error("Bind called without any buffers!");

    if (auto const texture = dynamic_cast<mir::graphics::gl::Texture*>(wrapped->native_buffer_base())) {
        texture->bind();
    } else {
        throw std::logic_error("Buffer does not support GL rendering");
    }
}
