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

#ifndef MIR_GRAPHICS_GRAPHIC_BUFFER_ALLOCATOR_H_
#define MIR_GRAPHICS_GRAPHIC_BUFFER_ALLOCATOR_H_

#include <mir/graphics/buffer.h>

#include <vector>
#include <memory>
#include <functional>

namespace mir
{

namespace renderer::software
{
class RWMappable;
}

namespace graphics
{
class DMABufBuffer;
class DMABufEGLProvider;

/**
 * Interface to graphic buffer allocation.
 */
class GraphicBufferAllocator
{
public:
    virtual ~GraphicBufferAllocator() = default;

    /**
     * The supported CPU-accessible buffer pixel formats.
     * \note: once the above is deprecated, this is really only (marginally) useful
     *        for the software allocation path.
     *        (and we could probably move software/cpu buffers up to libmirserver)
     */
    virtual std::vector<MirPixelFormat> supported_pixel_formats() = 0;

    /**
     * allocates a 'software' (cpu-accessible) buffer
     * note: gbm-kms and eglstreams use ShmBuffer, android uses ANW with software usage bits.
     */
    virtual std::shared_ptr<Buffer> alloc_software_buffer(geometry::Size size, MirPixelFormat) = 0;

    virtual auto buffer_from_shm(
        std::shared_ptr<renderer::software::RWMappable> shm_data,
        std::function<void()>&& on_consumed,
        std::function<void()>&& on_release) -> std::shared_ptr<Buffer> = 0;

    /**
     * Import a DMA-BUF buffer for use in compositing.
     *
     * \param dmabuf [in]       The DMA-BUF buffer to import
     * \param on_consumed [in]  Callback invoked when the buffer contents have been consumed
     * \param on_release [in]   Callback invoked when the buffer is no longer in use
     */
    virtual auto buffer_from_dmabuf(
        DMABufBuffer const& dmabuf,
        std::function<void()>&& on_consumed,
        std::function<void()>&& on_release) -> std::shared_ptr<Buffer> = 0;

    /**
     * Get the DMA-BUF EGL provider, if available.
     *
     * Used for linux-dmabuf protocol format negotiation and buffer import.
     * May return nullptr if the platform does not support DMA-BUF.
     */
    virtual auto dmabuf_provider() const -> std::shared_ptr<DMABufEGLProvider> { return nullptr; }

protected:
    GraphicBufferAllocator() = default;
    GraphicBufferAllocator(const GraphicBufferAllocator&) = delete;
    GraphicBufferAllocator& operator=(const GraphicBufferAllocator&) = delete;
};

}
}
#endif // MIR_GRAPHICS_GRAPHIC_BUFFER_ALLOCATOR_H_
