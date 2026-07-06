/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <miral/decoration_buffers.h>

#include <mir/fatal.h>
#include <mir/graphics/buffer.h>
#include <mir/graphics/graphic_buffer_allocator.h>
#include <mir/renderer/sw/pixel_source.h>

#include <utility>

namespace miral
{

struct DecorationBuffers::Self
{
    std::shared_ptr<mir::graphics::GraphicBufferAllocator> allocator;
};

struct DecorationBuffers::Mapping::Self
{
    Self(std::shared_ptr<mir::graphics::Buffer> const& buffer) :
        buffer{buffer},
        mappable{mir::renderer::software::as_write_mappable(buffer)},
        mapping{mappable->map_writeable()}
    {}

    std::shared_ptr<mir::graphics::Buffer> const buffer;
    std::shared_ptr<mir::renderer::software::WriteMappable> mappable;
    std::unique_ptr<mir::renderer::software::Mapping<std::byte>> mapping;
};

DecorationBuffers::Mapping::Mapping(std::shared_ptr<mir::graphics::Buffer> buffer) :
    self{std::make_unique<Self>(std::move(buffer))}
{}

DecorationBuffers::Mapping::~Mapping() = default;

DecorationBuffers::Mapping::Mapping(Mapping&&) noexcept = default;

auto DecorationBuffers::Mapping::operator=(Mapping&&) noexcept -> Mapping& = default;

auto DecorationBuffers::Mapping::data() -> uint8_t*
{
    return reinterpret_cast<uint8_t*>(self->mapping->data());
}

auto DecorationBuffers::Mapping::stride() const -> mir::geometry::Stride
{
    return self->mapping->stride();
}

auto DecorationBuffers::Mapping::size() const -> mir::geometry::Size
{
    return self->mapping->size();
}

auto DecorationBuffers::Mapping::format() const -> MirPixelFormat
{
    return self->mapping->format();
}

auto DecorationBuffers::Mapping::pixels32() -> uint32_t*
{
    return reinterpret_cast<uint32_t*>(self->mapping->data());
}

DecorationBuffers::DecorationBuffers(std::shared_ptr<mir::graphics::GraphicBufferAllocator> allocator) :
    self{std::make_unique<Self>(std::move(allocator))}
{}

DecorationBuffers::~DecorationBuffers() = default;

auto DecorationBuffers::create_mapping(mir::geometry::Size size, MirPixelFormat format) -> Pixels
{
    if (!self->allocator)
        mir::fatal_error("DecorationBuffers: no allocator available");

    auto const buffer = self->allocator->alloc_software_buffer(size, format);
    return Pixels{DecorationSurface{buffer}, Mapping{buffer}};
}

}
