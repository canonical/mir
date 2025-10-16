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

#ifndef MIR_RENDERER_SW_PIXEL_SOURCE_H_
#define MIR_RENDERER_SW_PIXEL_SOURCE_H_

#include <stddef.h>
#include <functional>
#include <memory>

#include <mir/geometry/dimensions.h>
#include <mir/geometry/size.h>
#include <mir/graphics/buffer.h>
#include <mir_toolkit/common.h>

namespace mir
{
namespace graphics
{
class GraphicBufferAllocator;
}
namespace renderer
{
namespace software
{
template<typename T>
class Mapping
{
public:
    /**
     * Destroy the mapping of the buffer.
     *
     * This does not destroy the underlying buffer, only the CPU-accessible
     * view of it.
     */
    virtual ~Mapping() = default;

    virtual T* data() = 0;
    virtual size_t len() const = 0;
    /// \returns The pixel format of the buffer
    virtual MirPixelFormat format() const = 0;
    /// \returns The stride (width + any padding)
    virtual geometry::Stride stride() const = 0;
    /// \returns The size of the buffer
    virtual geometry::Size size() const = 0;
};

/**
 * A Buffer that can be mapped into CPU-accessible memory and directly written to.
 */
class WriteMappable
{
public:
    virtual ~WriteMappable() = default;

    /**
     * Map the buffer into CPU-writeable memory.
     *
     * \note    The content of the mapping is undefined. In the absence of other guarantees, reading from unwritten
     *          areas of the mapping are undefined behaviour.
     * \note    The mapping may not be coherent with the GPU. Updates made to the mapping are guaranteed to be
     *          GPU-visible only once the mapping is destroyed.
     * \note    Pixels not written to while the mapping is active are left unchanged in the underlying buffer
     *          after unmap.
     * \return  A CPU-mapped view of the buffer.
     */
    virtual auto map_writeable() -> std::unique_ptr<Mapping<std::byte>> = 0;
    /// \returns The pixel format of the buffer
    virtual MirPixelFormat format() const = 0;
    /// \returns The stride (width + any padding)
    virtual geometry::Stride stride() const = 0;
    /// \returns The size of the buffer
    virtual geometry::Size size() const = 0;};

/**
 * A Buffer that can be mapped into CPU-readable memory.
 */
class ReadMappable
{
public:
    virtual ~ReadMappable() = default;

    /**
     * Map the buffer into CPU-readable memory.
     *
     * \note    The mapping may not be coherent with the GPU. Updates made by the GPU to the buffer after creation
     *          of the mapping are not guaranteed to be visible.
     * \return  A CPU-mapped view of the buffer.
     */
    virtual auto map_readable() -> std::unique_ptr<Mapping<std::byte const>> = 0;
    /// \returns The pixel format of the buffer
    virtual MirPixelFormat format() const = 0;
    /// \returns The stride (width + any padding)
    virtual geometry::Stride stride() const = 0;
    /// \returns The size of the buffer
    virtual geometry::Size size() const = 0;
};

/**
 * A buffer that can be mapped into CPU-accessible memory for both reading and writing.
 */
class RWMappable :
    public ReadMappable,
    public WriteMappable
{
public:
    ~RWMappable() override = default;

    /**
     * Map the buffer into CPU-accessible memory for both reading and writing.
     *
     * \note    While this has the same signature as \ref map_writable(), the content of the mapping is
     *          defined. Reads to unwritten areas will return the content of the buffer at map time.
     * \note    The mapping may not be coherent with the GPU. Updates made by the GPU to the buffer after creation
     *          of the mapping are not guaranteed to be visible in the mapping, nor are changes to the mapping
     *          guaranteed to be visible to the GPU until the mapping is destroyed.
     * \return A CPU-mapped view of the buffer.
     */
    virtual auto map_rw() -> std::unique_ptr<Mapping<std::byte>> = 0;
    /// \returns The pixel format of the buffer
    virtual MirPixelFormat format() const override = 0;
    /// \returns The stride (width + any padding)
    virtual geometry::Stride stride() const override = 0;
    /// \returns The size of the buffer
    virtual geometry::Size size() const override = 0;
};

class ReadTransferable
{
public:
    virtual ~ReadTransferable() = default;

    virtual void transfer_from_buffer(std::byte* destination) const = 0;
    /// \returns The pixel format of the buffer
    virtual MirPixelFormat format() const = 0;
    /// \returns The stride (width + any padding)
    virtual geometry::Stride stride() const = 0;
    /// \returns The size of the buffer
    virtual geometry::Size size() const = 0;
};

class WriteTransferable
{
public:
    virtual ~WriteTransferable() = default;

    virtual void transfer_into_buffer(std::byte const* source) = 0;

    /// \returns The pixel format of the buffer
    virtual MirPixelFormat format() const = 0;
    /// \returns The stride (width + any padding)
    virtual geometry::Stride stride() const = 0;
    /// \returns The size of the buffer
    virtual geometry::Size size() const = 0;
};

auto as_read_mappable(
    std::shared_ptr<graphics::Buffer> const& buffer) -> std::shared_ptr<ReadMappable>;

auto as_write_mappable(
    std::shared_ptr<graphics::Buffer> const& buffer) -> std::shared_ptr<WriteMappable>;

auto alloc_buffer_with_content(
    graphics::GraphicBufferAllocator& allocator,
    unsigned char const* content,
    geometry::Size const& size,
    geometry::Stride const& src_stride,
    MirPixelFormat src_format) -> std::shared_ptr<graphics::Buffer>;
}
}
}

#endif /* MIR_RENDERER_SW_PIXEL_SOURCE_H_ */
