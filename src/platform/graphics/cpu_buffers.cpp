/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "mir/renderer/sw/pixel_source.h"

#include <mir/graphics/graphic_buffer_allocator.h>

#include <boost/throw_exception.hpp>
#include <cstring>
#include <memory>
#include <stdexcept>

namespace mg = mir::graphics;
namespace mrs = mir::renderer::software;
namespace geom = mir::geometry;

namespace
{
template<
    typename BufferType,
    typename DataType,
    void(*Initialise)(BufferType&, std::byte*),
    void(*Finalise)(BufferType&, std::byte const*)>
class CopyMap : public mrs::Mapping<DataType>
{
public:
    CopyMap(std::shared_ptr<BufferType> buffer)
        : buffer{std::move(buffer)},
          bounce_buffer{
              std::make_unique<std::byte[]>(
                  this->buffer->stride().as_uint32_t() * this->buffer->size().height.as_uint32_t())}
    {
        Initialise(*this->buffer, bounce_buffer.get());
    }

    ~CopyMap()
    {
        Finalise(*buffer, bounce_buffer.get());
    }

    auto format() const -> MirPixelFormat override
    {
        return buffer->format();
    }

    auto stride() const -> mir::geometry::Stride override
    {
        return buffer->stride();
    }

    auto size() const -> mir::geometry::Size override
    {
        return buffer->size();
    }

    auto data() -> DataType* override
    {
        return bounce_buffer.get();
    }

    auto len() const -> size_t override
    {
        return stride().as_uint32_t() * size().height.as_uint32_t();
    }
private:
    std::shared_ptr<BufferType> const buffer;
    std::unique_ptr<std::byte[]> const bounce_buffer;
};

void read_from_buffer(mrs::ReadTransferable& buffer, std::byte* scratch_buffer)
{
    buffer.transfer_from_buffer(scratch_buffer);
}

void write_to_buffer(mrs::WriteTransferable& buffer, std::byte const* scratch_buffer)
{
    buffer.transfer_into_buffer(scratch_buffer);
}

template<typename Buffer, typename DataType>
void noop(Buffer&, DataType*)
{
}

}

auto mrs::as_read_mappable(
    std::shared_ptr<mg::Buffer> const& buffer) -> std::shared_ptr<ReadMappable>
{
    class CopyingWrapper : public ReadMappable
    {
    public:
        explicit CopyingWrapper(std::shared_ptr<ReadTransferable> underlying_buffer)
            : buffer{std::move(underlying_buffer)}
        {
        }

        std::unique_ptr<Mapping<std::byte const>> map_readable() override
        {
            return std::make_unique<
                CopyMap<
                    ReadTransferable,
                    std::byte const,
                    &read_from_buffer,
                    &noop>>(buffer);
        }

        auto format() const -> MirPixelFormat override { return buffer->format(); }
        auto stride() const -> geometry::Stride override { return buffer->stride(); }
        auto size() const -> geometry::Size override { return buffer->size(); }

    private:
        std::shared_ptr<ReadTransferable> const buffer;

    };

    if (auto mappable_buffer = dynamic_cast<ReadMappable*>(buffer->native_buffer_base()))
    {
        return std::shared_ptr<ReadMappable>{buffer, mappable_buffer};
    }
    else if (auto transferable_buffer = dynamic_cast<ReadTransferable*>(buffer->native_buffer_base()))
    {
        return std::make_shared<CopyingWrapper>(
            std::shared_ptr<ReadTransferable>{buffer, transferable_buffer});
    }

    BOOST_THROW_EXCEPTION((std::runtime_error{"Buffer does not support CPU access"}));
}

auto mrs::as_write_mappable(
    std::shared_ptr<mg::Buffer> const& buffer) -> std::shared_ptr<mrs::WriteMappable>
{
    class CopyingWrapper : public mrs::WriteMappable
    {
    public:
        explicit CopyingWrapper(std::shared_ptr<mrs::WriteTransferable> underlying_buffer)
            : buffer{std::move(underlying_buffer)}
        {
        }

        std::unique_ptr<mrs::Mapping<std::byte>> map_writeable() override
        {
            return std::make_unique<
                CopyMap<
                    mrs::WriteTransferable,
                    std::byte,
                    &noop,
                    &write_to_buffer>>(buffer);
        }

        auto format() const -> MirPixelFormat override { return buffer->format(); }
        auto stride() const -> geom::Stride override { return buffer->stride(); }
        auto size() const -> geom::Size override { return buffer->size(); }

    private:
        std::shared_ptr<mrs::WriteTransferable> const buffer;
    };

    if (auto mappable_buffer = dynamic_cast<mrs::WriteMappable*>(buffer->native_buffer_base()))
    {
        return std::shared_ptr<mrs::WriteMappable>{buffer, mappable_buffer};
    }
    else if (auto transferable_buffer = dynamic_cast<mrs::WriteTransferable*>(buffer->native_buffer_base()))
    {
        return std::make_shared<CopyingWrapper>(
            std::shared_ptr<mrs::WriteTransferable>{buffer, transferable_buffer});
    }
    BOOST_THROW_EXCEPTION((std::runtime_error{"Buffer does not support CPU access"}));
}

auto mrs::alloc_buffer_with_content(
    mg::GraphicBufferAllocator& allocator,
    unsigned char const* content,
    mir::geometry::Size const& size,
    mir::geometry::Stride const& src_stride,
    MirPixelFormat src_format) -> std::shared_ptr<graphics::Buffer>
{
    auto const buffer = allocator.alloc_software_buffer(size, src_format);

    auto mapping = as_write_mappable(buffer)->map_writeable();
    if (mapping->stride() == src_stride)
    {
        // Happy case: Buffer is packed, like the cursor_image; we can just blit.
        ::memcpy(mapping->data(), content, mapping->len());
    }
    else
    {
        // Less happy path: the buffer has a different stride; we need to copy row-by-row
        auto const dest_stride = mapping->stride().as_uint32_t();
        for (auto y = 0u; y < size.height.as_uint32_t(); ++y)
        {
            ::memcpy(
                mapping->data() + (dest_stride * y),
                content + (src_stride.as_uint32_t() * y),
                src_stride.as_uint32_t());
        }
    }
    return buffer;
}
