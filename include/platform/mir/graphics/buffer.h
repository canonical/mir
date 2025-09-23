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

#ifndef MIR_GRAPHICS_BUFFER_H_
#define MIR_GRAPHICS_BUFFER_H_

#include <memory>
#include <stdexcept>

#include "mir/graphics/buffer_id.h"
#include "mir/geometry/size.h"
#include "mir_toolkit/common.h"

namespace mir
{
namespace renderer::software
{
template<typename Data>
class Mapping;
}

namespace graphics
{

/// Used with buffers that wrap other buffers. This interface supports direct
/// access to the innermost buffer.
///
/// \sa #ClaimedBuffer - A buffer wrapper that invokes a callback on destruction.
class NativeBufferBase
{
protected:
    NativeBufferBase() = default;
    virtual ~NativeBufferBase() = default;
    NativeBufferBase(NativeBufferBase const&) = delete;
    NativeBufferBase operator=(NativeBufferBase const&) = delete;
};

class UnmappableBuffer : public std::invalid_argument
{
public:
    using std::invalid_argument::invalid_argument;
};

/// Generic graphics buffer interface.
///
/// Buffers can come from Wayland resources, shared memory, DMA buffers. CPU
/// accessible buffers can also be created by the compositor for internal
/// rendering.
///
/// Some buffer types can be converted to #mir::graphics::Framebuffer for display, or
/// #mir::graphics::gl::Texture for compositing.
///
///
/// \sa
/// - GraphicBufferAllocator - Allocates different types of buffers
/// - DMABufEGLProvider::import_dma_buf
/// - FramebufferProvider::buffer_to_framebuffer
/// - GLRenderingProvider::as_texture - Convert a buffer to an OpenGL texture
class Buffer
{
public:
    virtual ~Buffer() = default;

    /// \returns The unique ID of this buffer.
    virtual auto id() const -> BufferID = 0;

    /// \returns The size of the buffer.
    ///
    /// \note The size is the logical size of the buffer, where the width is
    /// the perceived width, and NOT the stride.
    virtual auto size() const -> geometry::Size = 0;

    /// The pixel format determines how the pixels are laid out in memory, and
    /// whether or not the buffer supports transparency.
    ///
    /// \returns The pixel format of the buffer.
    virtual auto pixel_format() const -> MirPixelFormat = 0;

    /// Used with buffer classes that wrap other buffers.
    ///
    /// \returns A direct pointer to the inner/wrapped buffer.
    virtual auto native_buffer_base() -> NativeBufferBase* = 0;

    /// Access the buffer content from the CPU
    ///
    /// \note This access might be costly (for example, requiring a copy across the PCIe bus
    //        for buffers in VRAM).
    ///       Wherever possible, buffers should be accessed through rendering-API-specific means,
    ///       such as [GLRenderingProvider::as_texture].
    ///
    /// \returns A [Mapping] describing a CPU-accessible view of the buffer. The lifetime of
    ///          this [Mapping] is tied to the lifetime of this [Buffer].
    ///
    /// \throws An [UnmappableBuffer] if the buffer cannot be mapped in this way. This may
    ///         occur if the buffer has a format not representable by [MirPixelFormat]
    ///         (for example, a planar YUV format) or if the buffer is being protected by the
    ///         GPU Digital Rights Management path.
    /// \throws A std::system_error if mapping
    virtual auto map_readable() const -> std::unique_ptr<renderer::software::Mapping<std::byte const>> = 0;
protected:
    Buffer() = default;
};

}
}
#endif // MIR_GRAPHICS_BUFFER_H_
