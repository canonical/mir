/*
 * Copyright © 2016 Canonical Ltd.
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_RENDERER_SW_PIXEL_SOURCE_H_
#define MIR_RENDERER_SW_PIXEL_SOURCE_H_

#include <stddef.h>
#include <functional>
#include <memory>

#include "mir/geometry/dimensions.h"
#include "mir/geometry/size.h"
#include "mir/graphics/buffer.h"
#include "mir_toolkit/common.h"

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

class BufferDescriptor
{
public:
    virtual ~BufferDescriptor() = default;

    virtual MirPixelFormat format() const = 0;
    virtual geometry::Stride stride() const = 0;
    virtual geometry::Size size() const = 0;
};

template<typename T>
class Mapping : public BufferDescriptor
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
};

/**
 * A Buffer that can be mapped into CPU-accessible memory and directly written to.
 */
class WriteMappableBuffer
{
public:
    virtual ~WriteMappableBuffer() = default;

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
    virtual std::unique_ptr<Mapping<unsigned char>> map_writeable() = 0;
};

/**
 * A Buffer that can be mapped into CPU-readable memory.
 */
class ReadMappableBuffer
{
public:
    virtual ~ReadMappableBuffer() = default;

    /**
     * Map the buffer into CPU-readable memory.
     *
     * \note    The mapping may not be coherent with the GPU. Updates made by the GPU to the buffer after creation
     *          of the mapping are not guaranteed to be visible.
     * \return  A CPU-mapped view of the buffer.
     */
    virtual std::unique_ptr<Mapping<unsigned char const>> map_readable() = 0;
};

/**
 * A buffer that can be mapped into CPU-accessible memory for both reading and writing.
 */
class RWMappableBuffer :
    public ReadMappableBuffer,
    public WriteMappableBuffer
{
public:
    ~RWMappableBuffer() override = default;

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
    virtual std::unique_ptr<Mapping<unsigned char>> map_rw() = 0;
};

class ReadTransferableBuffer : public virtual BufferDescriptor
{
public:
    virtual ~ReadTransferableBuffer() = default;

    virtual void transfer_from_buffer(unsigned char* destination) const = 0;
};

class WriteTransferableBuffer : public virtual BufferDescriptor
{
public:
    virtual ~WriteTransferableBuffer() = default;

    virtual void transfer_into_buffer(unsigned char const* source) = 0;
};

auto as_read_mappable_buffer(
    std::shared_ptr<graphics::Buffer> const& buffer) -> std::shared_ptr<ReadMappableBuffer>;

auto alloc_buffer_with_content(
    graphics::GraphicBufferAllocator& allocator,
    unsigned char const* content,
    geometry::Size const& size,
    geometry::Stride const& src_stride,
    MirPixelFormat src_format) -> std::shared_ptr<graphics::Buffer>;

class PixelSource
{
public:
    virtual ~PixelSource() = default;

    //functions have legacy issues with their signatures. 
    //FIXME: correct write, it requires that the user does too much to use it correctly,
    //       (ie, it forces them to figure out what size is proper, alloc a buffer, fill it, and then
    //       copy the data into the buffer)
    virtual void write(unsigned char const* pixels, size_t size) = 0;
    //FIXME: correct read, it doesn't give size or format information about the pixels.
    virtual void read(std::function<void(unsigned char const*)> const& do_with_pixels) = 0;
    virtual geometry::Stride stride() const = 0;

protected:
    PixelSource() = default;
    PixelSource(PixelSource const&) = delete;
    PixelSource& operator=(PixelSource const&) = delete;
};

}
}
}

#endif /* MIR_RENDERER_SW_PIXEL_SOURCE_H_ */
