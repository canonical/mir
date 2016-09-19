/*
 * Copyright © 2016 Canonical Ltd.
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir/renderer/sw/pixel_source.h"
#include "mir/renderer/gl/texture_source.h"
#include "mir/graphics/egl_extensions.h"
#include "mir_toolkit/mir_buffer.h"
#include "host_connection.h"
#include "buffer.h"
#include "egl_image_factory.h"
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
    public mrg::TextureSource
{
public:
    TextureAccess(
        mgn::Buffer& buffer,
        std::shared_ptr<mgn::NativeBuffer> const& native_buffer,
        std::shared_ptr<mgn::HostConnection> const& connection,
        std::shared_ptr<mgn::EglImageFactory> const& factory) :
        buffer(buffer),
        native_buffer(native_buffer),
        connection(connection),
        factory(factory)
    {
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
            static const EGLint image_attrs[] = { EGL_IMAGE_PRESERVED_KHR, EGL_TRUE, EGL_NONE };
            auto i = factory->create_egl_image_from(*native_buffer, resources.first, image_attrs);
            image = *i;
            egl_image_map[resources] = std::move(i);
        }
        else
        {
            image = *(it->second);
        }

        egl_extensions.glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, image);
    }

    void gl_bind_to_texture() override
    {
        bind();
    }

    void secure_for_render() override
    {
    }

protected:
    mgn::Buffer& buffer;
    std::shared_ptr<mgn::NativeBuffer> const native_buffer;
    std::shared_ptr<mgn::HostConnection> const connection;
private:
    std::shared_ptr<mgn::EglImageFactory> const factory;
    mg::EGLExtensions egl_extensions;
    typedef std::pair<EGLDisplay, EGLContext> ImageResources;
    std::map<ImageResources, std::unique_ptr<EGLImageKHR>> egl_image_map;
};

class PixelAndTextureAccess :
    public TextureAccess,
    public mrs::PixelSource
{
public:
    PixelAndTextureAccess(
        mgn::Buffer& buffer,
        std::shared_ptr<mgn::NativeBuffer> const& native_buffer,
        std::shared_ptr<mgn::HostConnection> const& connection,
        std::shared_ptr<mgn::EglImageFactory> const& factory) :
        TextureAccess(buffer, native_buffer, connection, factory),
        stride_(geom::Stride{native_buffer->get_graphics_region().stride})
    {
    }

    void write(unsigned char const* pixels, size_t pixel_size) override
    {
        auto bpp = MIR_BYTES_PER_PIXEL(buffer.pixel_format());
        size_t buffer_size_bytes = buffer.size().height.as_int() * buffer.size().width.as_int() * bpp;
        if (buffer_size_bytes != pixel_size)
            BOOST_THROW_EXCEPTION(std::logic_error("Size of pixels is not equal to size of buffer"));

        auto region = native_buffer->get_graphics_region();
        if (!region.vaddr)
            BOOST_THROW_EXCEPTION(std::logic_error("could not map buffer"));

        for (int i = 0; i < region.height; i++)
        {
            int line_offset_in_buffer = stride().as_uint32_t() * i;
            int line_offset_in_source = bpp * region.width * i;
            memcpy(region.vaddr + line_offset_in_buffer, pixels + line_offset_in_source, region.width * bpp);
        }
    }

    void read(std::function<void(unsigned char const*)> const& do_with_pixels) override
    {
        auto region = native_buffer->get_graphics_region();
        do_with_pixels(reinterpret_cast<unsigned char*>(region.vaddr));
    }

    geom::Stride stride() const override
    {
        return stride_;
    }

private:
    geom::Stride const stride_;
};
}

std::shared_ptr<mg::NativeBufferBase> mgn::Buffer::create_native_base(mg::BufferUsage const usage)
{
    if (usage == mg::BufferUsage::software)
        return std::make_shared<PixelAndTextureAccess>(*this, buffer, connection, factory);
    else if (usage == mg::BufferUsage::hardware)
        return std::make_shared<TextureAccess>(*this, buffer, connection, factory);
    else
        BOOST_THROW_EXCEPTION(std::invalid_argument("usage not supported when creating nested::Buffer"));
}

mgn::Buffer::Buffer(
    std::shared_ptr<HostConnection> const& connection,
    std::shared_ptr<EglImageFactory> const& factory,
    mg::BufferProperties const& properties) :
    connection(connection),
    factory(factory),
    buffer(connection->create_buffer(properties)),
    native_base(create_native_base(properties.usage))
{
}

std::shared_ptr<mg::NativeBuffer> mgn::Buffer::native_buffer_handle() const
{
    return nullptr;
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
