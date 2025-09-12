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

#include "mir/graphics/buffer_id.h"
#include "mir/geometry/size.h"
#include "mir_toolkit/common.h"

namespace mir
{
namespace graphics
{

class NativeBufferBase
{
protected:
    NativeBufferBase() = default;
    virtual ~NativeBufferBase() = default;
    NativeBufferBase(NativeBufferBase const&) = delete;
    NativeBufferBase operator=(NativeBufferBase const&) = delete;
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
/// \sa GraphicBufferAllocator::buffer_from_resource
/// \sa GraphicBufferAllocator::buffer_from_shm
/// \sa DMABufEGLProvider::import_dma_buf
/// \sa GraphicBufferAllocator::alloc_software_buffer
/// \sa FramebufferProvider::buffer_to_framebuffer
/// \sa GLRenderingProvider::as_texture
class Buffer
{
public:
    virtual ~Buffer() {}

    /// The unique ID of this buffer.
    ///
    /// This ID is mainly used to identify the buffer for tracing. Returning 0
    /// or any arbitrary value does not affect normal operation of the
    /// compositor.
    ///
    /// \returns The unique ID of this buffer.
    virtual BufferID id() const = 0;

    /// \note The size is the logical size of the buffer, where the width is
    /// the perceived width, and NOT the stride.
    ///
    /// \returns The size of the buffer.
    virtual geometry::Size size() const = 0;

    /// The pixel format determines how the pixels are laid out in memory, and
    /// whether or not the buffer supports transparency.
    ///
    /// \returns The pixel format of the buffer.
    virtual MirPixelFormat pixel_format() const = 0;

    /// Used with buffer classes that wrap other buffers.
    ///
    /// \returns A direct pointer to the inner/wrapped buffer.
    virtual NativeBufferBase* native_buffer_base() = 0;

protected:
    Buffer() = default;
};

}
}
#endif // MIR_GRAPHICS_BUFFER_H_
