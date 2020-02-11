/*
 * Copyright © 2012-2014 Canonical Ltd.
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

#ifndef MIR_TEST_DOUBLES_STUB_BUFFER_H_
#define MIR_TEST_DOUBLES_STUB_BUFFER_H_

#include "mir/graphics/buffer_basic.h"
#include "mir/graphics/buffer_properties.h"
#include "mir/geometry/size.h"
#include "mir/graphics/buffer_id.h"
#include "mir/renderer/sw/pixel_source.h"
#include <vector>
#include <string.h>

#include <boost/throw_exception.hpp>
#include <exception>
#include <stdexcept>

namespace mir
{
namespace test
{
namespace doubles
{

class StubBuffer :
    public graphics::BufferBasic,
    public graphics::NativeBufferBase,
    public renderer::software::PixelSource
{
public:
    StubBuffer()
        : StubBuffer{
              nullptr,
              graphics::BufferProperties{
                  geometry::Size{},
                  mir_pixel_format_abgr_8888,
                  graphics::BufferUsage::hardware},
              geometry::Stride{}}

    {
    }

    StubBuffer(geometry::Size const& size)
        : StubBuffer{
              nullptr,
              graphics::BufferProperties{
                  size,
                  mir_pixel_format_abgr_8888,
                  graphics::BufferUsage::hardware},
              geometry::Stride{}}

    {
    }

    StubBuffer(std::shared_ptr<graphics::NativeBuffer> const& native_buffer, geometry::Size const& size, MirPixelFormat const pixel_format)
        : StubBuffer{
              native_buffer,
              graphics::BufferProperties{
                  size,
                  pixel_format,
                  graphics::BufferUsage::hardware},
                  geometry::Stride{}}

    {
    }

    StubBuffer(std::shared_ptr<graphics::NativeBuffer> const& native_buffer)
        : StubBuffer{native_buffer, {}, mir_pixel_format_abgr_8888}
    {
    }

    StubBuffer(graphics::BufferProperties const& properties)
        : StubBuffer{nullptr, properties, geometry::Stride{properties.size.width.as_int() * MIR_BYTES_PER_PIXEL(properties.format)}}
    {
    }

    StubBuffer(graphics::BufferID id)
        : native_buffer(nullptr),
          buf_size{},
          buf_pixel_format{mir_pixel_format_abgr_8888},
          buf_stride{},
          buf_id{id}
    {
    }

    StubBuffer(std::shared_ptr<graphics::NativeBuffer> const& native_buffer,
               graphics::BufferProperties const& properties,
               geometry::Stride stride)
        : native_buffer(native_buffer),
          buf_size{properties.size},
          buf_pixel_format{properties.format},
          buf_stride{stride},
          buf_id{graphics::BufferBasic::id()}
    {
    }

    virtual graphics::BufferID id() const override { return buf_id; }

    virtual geometry::Size size() const override { return buf_size; }

    virtual geometry::Stride stride() const override { return buf_stride; }

    virtual MirPixelFormat pixel_format() const override { return buf_pixel_format; }

    virtual std::shared_ptr<graphics::NativeBuffer> native_buffer_handle() const override
    {
        if (native_buffer)
            return native_buffer;
        BOOST_THROW_EXCEPTION(std::runtime_error("cannot access native buffer"));
    }

    void write(unsigned char const* pixels, size_t len) override
    {
        if (pixels) written_pixels.assign(pixels, pixels + len);
    }
    void read(std::function<void(unsigned char const*)> const& do_with_pixels) override
    {
        if (written_pixels.size() == 0)
        {
            auto length = buf_size.width.as_int()*buf_size.height.as_int()*MIR_BYTES_PER_PIXEL(buf_pixel_format);
            written_pixels.resize(length);
            memset(written_pixels.data(), 0, length);
        }
        do_with_pixels(written_pixels.data());
    }

    NativeBufferBase* native_buffer_base() override
    {
        return this;
    }

    std::shared_ptr<graphics::NativeBuffer> const native_buffer;
    geometry::Size const buf_size;
    MirPixelFormat const buf_pixel_format;
    geometry::Stride const buf_stride;
    graphics::BufferID const buf_id;
    std::vector<unsigned char> written_pixels;
};
}
}
}
#endif /* MIR_TEST_DOUBLES_STUB_BUFFER_H_ */
