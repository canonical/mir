/*
 * Copyright © Canonical Ltd.
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

#ifndef MIR_TEST_DOUBLES_STUB_BUFFER_H_
#define MIR_TEST_DOUBLES_STUB_BUFFER_H_

#include "mir/graphics/buffer_basic.h"
#include "mir/graphics/buffer_properties.h"
#include "mir/geometry/size.h"
#include "mir/graphics/buffer_id.h"
#include "mir/renderer/sw/pixel_source.h"
#include <vector>
#include <string.h>

namespace mir
{
namespace test
{
namespace doubles
{

class StubBuffer :
    public graphics::BufferBasic,
    public graphics::NativeBufferBase,
    public renderer::software::RWMappableBuffer
{
public:
    StubBuffer()
        : StubBuffer{
              graphics::BufferProperties{
                  geometry::Size{},
                  mir_pixel_format_abgr_8888,
                  graphics::BufferUsage::hardware},
              geometry::Stride{}}

    {
    }

    StubBuffer(geometry::Size const& size)
        : StubBuffer{
              graphics::BufferProperties{
                  size,
                  mir_pixel_format_abgr_8888,
                  graphics::BufferUsage::hardware},
              geometry::Stride{}}

    {
    }

    StubBuffer(geometry::Size const& size, MirPixelFormat const pixel_format)
        : StubBuffer{
              graphics::BufferProperties{
                  size,
                  pixel_format,
                  graphics::BufferUsage::hardware},
                  geometry::Stride{}}

    {
    }

    StubBuffer(graphics::BufferProperties const& properties)
        : StubBuffer{properties, geometry::Stride{properties.size.width.as_int() * MIR_BYTES_PER_PIXEL(properties.format)}}
    {
        written_pixels.resize(buf_size.height.as_uint32_t() * buf_stride.as_uint32_t());
        if (written_pixels.size())
        {
            // vector<>::data() is permitted to return nullptr if size() == 0
            ::memset(written_pixels.data(), 0, written_pixels.size());
        }
    }

    StubBuffer(graphics::BufferID id)
        : buf_size{},
          buf_pixel_format{mir_pixel_format_abgr_8888},
          buf_stride{},
          buf_id{id}
    {
    }

    StubBuffer(graphics::BufferProperties const& properties,
               geometry::Stride stride)
        : buf_size{properties.size},
          buf_pixel_format{properties.format},
          buf_stride{stride},
          buf_id{graphics::BufferBasic::id()}
    {
        written_pixels.resize(buf_size.height.as_uint32_t() * buf_stride.as_uint32_t());
        if (written_pixels.size())
        {
            // vector<>::data() is permitted to return nullptr if size() == 0
            ::memset(written_pixels.data(), 0, written_pixels.size());
        }
    }

    virtual graphics::BufferID id() const override { return buf_id; }

    virtual geometry::Size size() const override { return buf_size; }

    virtual MirPixelFormat pixel_format() const override { return buf_pixel_format; }

    auto format() const -> MirPixelFormat override { return buf_pixel_format; }

    auto stride() const -> geometry::Stride override { return buf_stride; }

    template<typename T>
    class Mapping : public mir::renderer::software::Mapping<T>
    {
    public:
        Mapping(StubBuffer* buffer)
            : buffer{buffer}
        {
        }

        auto format() const -> MirPixelFormat override
        {
            return buffer->buf_pixel_format;
        }

        auto stride() const -> geometry::Stride override
        {
            return buffer->buf_stride;
        }

        auto size() const -> geometry::Size override
        {
            return buffer->buf_size;
        }

        auto data() -> T* override
        {
            return buffer->written_pixels.data();
        }

        auto len() const -> size_t override
        {
            return buffer->written_pixels.size();
        }

        void mark_dirty() override
        {
        }
    private:
        StubBuffer* const buffer;
    };

    template<typename T>
    friend class Mapping;

    auto map_writeable() -> std::unique_ptr<renderer::software::Mapping<unsigned char>> override
    {
        return std::make_unique<Mapping<unsigned char>>(this);
    }

    auto map_readable() -> std::unique_ptr<renderer::software::Mapping<unsigned char const>> override
    {
        return std::make_unique<Mapping<unsigned char const>>(this);
    }

    auto map_rw() -> std::unique_ptr<renderer::software::Mapping<unsigned char>> override
    {
        return std::make_unique<Mapping<unsigned char>>(this);
    }

    NativeBufferBase* native_buffer_base() override
    {
        return this;
    }

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
