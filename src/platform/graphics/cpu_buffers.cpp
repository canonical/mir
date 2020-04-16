/*
 * Copyright © 2020 Canonical Ltd.
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
 *
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include "mir/renderer/sw/pixel_source.h"

#include <algorithm>
#include <boost/throw_exception.hpp>
#include <cstring>
#include <memory>
#include <mir/graphics/graphic_buffer_allocator.h>
#include <stdexcept>

namespace mg = mir::graphics;
namespace mrs = mir::renderer::software;
namespace geom = mir::geometry;

namespace
{
template<
    typename BufferType,
    typename DataType,
    void(*Initialise)(BufferType&, unsigned char*),
    void(*Finalise)(BufferType&, unsigned char const*)>
class CopyMap : public mrs::Mapping<DataType>
{
public:
    CopyMap(std::shared_ptr<BufferType> buffer)
        : buffer{std::move(buffer)},
          bounce_buffer{
              std::make_unique<unsigned char[]>(
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
    std::unique_ptr<unsigned char[]> const bounce_buffer;
};

void read_from_buffer(mrs::ReadTransferableBuffer& buffer, unsigned char* scratch_buffer)
{
    buffer.transfer_from_buffer(scratch_buffer);
}

void write_to_buffer(mrs::WriteTransferableBuffer& buffer, unsigned char const* scratch_buffer)
{
    buffer.transfer_into_buffer(scratch_buffer);
}

template<typename Buffer, typename DataType>
void noop(Buffer&, DataType*)
{
}

}

auto mrs::as_read_mappable_buffer(
    std::shared_ptr<mg::Buffer> const& buffer) -> std::shared_ptr<ReadMappableBuffer>
{
    class CopyingWrapper : public ReadMappableBuffer
    {
    public:
        explicit CopyingWrapper(std::shared_ptr<ReadTransferableBuffer> underlying_buffer)
            : buffer{std::move(underlying_buffer)}
        {
        }

        std::unique_ptr<Mapping<unsigned char const>> map_readable() override
        {
            return std::make_unique<
                CopyMap<
                    ReadTransferableBuffer,
                    unsigned char const,
                    &read_from_buffer,
                    &noop>>(buffer);
        }

    private:
        std::shared_ptr<ReadTransferableBuffer> const buffer;

    };

    if (auto mappable_buffer = dynamic_cast<ReadMappableBuffer*>(buffer->native_buffer_base()))
    {
        return std::shared_ptr<ReadMappableBuffer>{buffer, mappable_buffer};
    }
    else if (auto transferable_buffer = dynamic_cast<ReadTransferableBuffer*>(buffer->native_buffer_base()))
    {
        return std::make_shared<CopyingWrapper>(
            std::shared_ptr<ReadTransferableBuffer>{buffer, transferable_buffer});
    } else if (dynamic_cast<PixelSource*>(buffer->native_buffer_base()))
    {
        class PixelSourceAdaptor : public ReadTransferableBuffer
        {
        public:
            explicit PixelSourceAdaptor(std::shared_ptr<mg::Buffer> buffer)
                : buffer{std::move(buffer)}
            {
                if (!dynamic_cast<PixelSource*>(this->buffer->native_buffer_base()))
                {
                    BOOST_THROW_EXCEPTION((
                        std::logic_error{
                            "Attempt to create a PixelSourceAdaptor for a non-PixelSource buffer!"}));
                }
            }

            MirPixelFormat format() const override
            {
                return buffer->pixel_format();
            }

            geometry::Stride stride() const override
            {
                return dynamic_cast<PixelSource const&>(*buffer->native_buffer_base()).stride();
            }

            geometry::Size size() const override
            {
                return buffer->size();
            }

            void transfer_from_buffer(unsigned char* destination) const override
            {
                dynamic_cast<PixelSource&>(*buffer->native_buffer_base()).read(
                    [length = stride().as_uint32_t() * size().height.as_uint32_t(), destination](auto pixels)
                    {
                        ::memcpy(destination, pixels, length);
                    });
            }
        private:
            std::shared_ptr<mg::Buffer> const buffer;
        };

        return std::make_shared<CopyingWrapper>(
            std::make_shared<PixelSourceAdaptor>(buffer));

    }

    BOOST_THROW_EXCEPTION((std::runtime_error{"Buffer does not support CPU access"}));
}

namespace
{
auto as_write_mappable_buffer(
    std::shared_ptr<mg::Buffer> const& buffer) -> std::shared_ptr<mrs::WriteMappableBuffer>
{
    class CopyingWrapper : public mrs::WriteMappableBuffer
    {
    public:
        explicit CopyingWrapper(std::shared_ptr<mrs::WriteTransferableBuffer> underlying_buffer)
            : buffer{std::move(underlying_buffer)}
        {
        }

        std::unique_ptr<mrs::Mapping<unsigned char>> map_writeable() override
        {
            return std::make_unique<
                CopyMap<
                    mrs::WriteTransferableBuffer,
                    unsigned char,
                    &noop,
                    &write_to_buffer>>(buffer);
        }

    private:
        std::shared_ptr<mrs::WriteTransferableBuffer> const buffer;
    };

    if (auto mappable_buffer = dynamic_cast<mrs::WriteMappableBuffer*>(buffer->native_buffer_base()))
    {
        return std::shared_ptr<mrs::WriteMappableBuffer>{buffer, mappable_buffer};
    }
    else if (auto transferable_buffer = dynamic_cast<mrs::WriteTransferableBuffer*>(buffer->native_buffer_base()))
    {
        return std::make_shared<CopyingWrapper>(
            std::shared_ptr<mrs::WriteTransferableBuffer>{buffer, transferable_buffer});
    }
    else if (dynamic_cast<mrs::PixelSource*>(buffer->native_buffer_base()))
    {
        class PixelSourceAdaptor : public mrs::WriteTransferableBuffer
        {
        public:
            explicit PixelSourceAdaptor(std::shared_ptr<mg::Buffer> buffer)
                : buffer{std::move(buffer)}
            {
                if (!dynamic_cast<mrs::PixelSource*>(this->buffer->native_buffer_base()))
                {
                    BOOST_THROW_EXCEPTION(
                        (std::logic_error{"Attempt to create a PixelSourceAdaptor for a non-PixelSource buffer!"}));
                }
            }

            MirPixelFormat format() const override
            {
                return buffer->pixel_format();
            }

            geom::Stride stride() const override
            {
                return dynamic_cast<mrs::PixelSource const&>(*buffer->native_buffer_base()).stride();
            }

            geom::Size size() const override
            {
                return buffer->size();
            }

            void transfer_into_buffer(unsigned char const* source) override
            {
                dynamic_cast<mrs::PixelSource&>(*buffer->native_buffer_base())
                    .write(source, stride().as_uint32_t() * size().height.as_uint32_t());
            }

        private:
            std::shared_ptr<mg::Buffer> const buffer;
        };

        return std::make_shared<CopyingWrapper>(std::make_shared<PixelSourceAdaptor>(buffer));
    }

    BOOST_THROW_EXCEPTION((std::runtime_error{"Buffer does not support CPU access"}));
}
}

auto mrs::alloc_buffer_with_content(
    mg::GraphicBufferAllocator& allocator,
    unsigned char const* content,
    mir::geometry::Size const& size,
    mir::geometry::Stride const& src_stride,
    MirPixelFormat src_format) -> std::shared_ptr<graphics::Buffer>
{
    auto const buffer = allocator.alloc_software_buffer(size, src_format);

    auto mapping = as_write_mappable_buffer(buffer)->map_writeable();
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

