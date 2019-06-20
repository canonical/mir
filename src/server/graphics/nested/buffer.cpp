/*
 * Copyright © 2016 Canonical Ltd.
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir/renderer/sw/pixel_source.h"
#include "mir/renderer/gl/texture_source.h"
#include "mir/renderer/gl/texture_target.h"
#include "mir/graphics/egl_extensions.h"
#include "mir/graphics/buffer_ipc_message.h"
#include "mir/graphics/gl_format.h"
#include "mir_toolkit/mir_buffer.h"
#include "host_connection.h"
#include "buffer.h"
#include "native_buffer.h"

#include <map>
#include <chrono>
#include <string.h>
#include <boost/throw_exception.hpp>
#include <stdexcept>

namespace mg = mir::graphics;
namespace mgn = mir::graphics::nested;
namespace mrs = mir::renderer::software;
namespace mrg = mir::renderer::gl;
namespace geom = mir::geometry;

namespace
{
class TextureAccess :
    public mg::NativeBufferBase,
    public mrg::TextureSource,
    public mrg::TextureTarget
{
public:
    TextureAccess(
        std::shared_ptr<mgn::NativeBuffer> const& native_buffer,
        std::shared_ptr<mgn::HostConnection> const& connection) :
        native_buffer(native_buffer),
        connection(connection)
    {
    }

    ~TextureAccess() {
    	for(auto& it : egl_image_map)
    	{
            EGLDisplay disp = it.first.first;
            extensions.eglDestroyImageKHR(disp, it.second);
    	}
    }

    void bind() override
    {
        using namespace std::chrono;
        native_buffer->sync(mir_read, duration_cast<nanoseconds>(seconds(1)));

        ImageResources resources
        {
            eglGetCurrentDisplay(),
            eglGetCurrentContext()
        };

        EGLImageKHR image;
        auto it = egl_image_map.find(resources);
        if (it == egl_image_map.end())
        {
            auto hints = native_buffer->egl_image_creation_hints();
            image = extensions.eglCreateImageKHR(
                    resources.first, EGL_NO_CONTEXT,
                    std::get<0>(hints), std::get<1>(hints), std::get<2>(hints));
            egl_image_map[resources] = image;
        }
        else
        {
            image = it->second;
        }

        extensions.glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, image);
    }

    void gl_bind_to_texture() override
    {
        bind();
    }

    void secure_for_render() override
    {
    }

    void bind_for_write() override
    {
        bind();
    }

    void commit() override
    {
    }

private:
    std::shared_ptr<mgn::NativeBuffer> const native_buffer;
    std::shared_ptr<mgn::HostConnection> const connection;
    mg::EGLExtensions extensions;
    typedef std::pair<EGLDisplay, EGLContext> ImageResources;
    std::map<ImageResources, EGLImageKHR> egl_image_map;
};

class PixelAndTextureAccess :
    public mg::NativeBufferBase,
    public mrs::PixelSource,
    public mrg::TextureSource
{
public:
    PixelAndTextureAccess(
        mgn::Buffer& buffer,
        std::shared_ptr<mgn::NativeBuffer> const& native_buffer) :
        buffer(buffer),
        native_buffer(native_buffer),
        stride_(geom::Stride{native_buffer->get_graphics_region()->stride})
    {
        if (!mg::get_gl_pixel_format(buffer.pixel_format(), format, type))
            BOOST_THROW_EXCEPTION(std::logic_error("buffer cannot be used as texture"));
    }

    void write(unsigned char const* pixels, size_t pixel_size) override
    {
        auto bpp = MIR_BYTES_PER_PIXEL(buffer.pixel_format());
        size_t buffer_size_bytes = buffer.size().height.as_int() * buffer.size().width.as_int() * bpp;
        if (buffer_size_bytes != pixel_size)
            BOOST_THROW_EXCEPTION(std::logic_error("Size of pixels is not equal to size of buffer"));

        auto region = native_buffer->get_graphics_region();
        if (!region->vaddr)
            BOOST_THROW_EXCEPTION(std::logic_error("could not map buffer"));

        for (int i = 0; i < region->height; i++)
        {
            int line_offset_in_buffer = stride().as_uint32_t() * i;
            int line_offset_in_source = bpp * region->width * i;
            memcpy(region->vaddr + line_offset_in_buffer, pixels + line_offset_in_source, region->width * bpp);
        }
    }

    void read(std::function<void(unsigned char const*)> const& do_with_pixels) override
    {
        auto region = native_buffer->get_graphics_region();
        do_with_pixels(reinterpret_cast<unsigned char*>(region->vaddr));
    }

    geom::Stride stride() const override
    {
        return stride_;
    }

    void bind() override
    {
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(
            GL_TEXTURE_2D, 0, format,
            buffer.size().width.as_int(), buffer.size().height.as_int(),
            0, format, type, native_buffer->get_graphics_region()->vaddr);
    }

    void gl_bind_to_texture() override
    {
        bind();
    }

    void secure_for_render() override
    {
    }

private:
    mgn::Buffer& buffer;
    std::shared_ptr<mgn::NativeBuffer> const native_buffer;
    geom::Stride const stride_;
    GLenum format;
    GLenum type;
};
}

std::shared_ptr<mg::NativeBufferBase> mgn::Buffer::create_native_base(mg::BufferUsage const usage)
{
    if (usage == mg::BufferUsage::software)
        return std::make_shared<PixelAndTextureAccess>(*this, buffer);
    else if (usage == mg::BufferUsage::hardware)
        return std::make_shared<TextureAccess>(buffer, connection);
    else
        BOOST_THROW_EXCEPTION(std::invalid_argument("usage not supported when creating nested::Buffer"));
}

mgn::Buffer::Buffer(
    std::shared_ptr<HostConnection> const& connection,
    mg::BufferProperties const& properties) :
    connection(connection),
    buffer(connection->create_buffer(properties)),
    native_base(create_native_base(properties.usage))
{
}

mgn::Buffer::Buffer(
    std::shared_ptr<HostConnection> const& connection,
    geom::Size size, uint32_t native_format, uint32_t native_flags) :
    connection(connection),
    buffer(connection->create_buffer(size, native_format, native_flags)),
    native_base(std::make_shared<TextureAccess>(buffer, connection))
{
}

mgn::Buffer::Buffer(
    std::shared_ptr<HostConnection> const& connection,
    geom::Size size,
    MirPixelFormat format) :
    connection(connection),
    buffer(connection->create_buffer(size, format)),
    native_base(std::make_shared<PixelAndTextureAccess>(*this, buffer))
{
}

std::shared_ptr<mg::NativeBuffer> mgn::Buffer::native_buffer_handle() const
{
    return buffer;
}

geom::Size mgn::Buffer::size() const
{
    return buffer->size();
}

MirPixelFormat mgn::Buffer::pixel_format() const
{
    return buffer->format();
}

mg::NativeBufferBase* mgn::Buffer::native_buffer_base()
{
    return native_base.get();
}
