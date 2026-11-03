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

#ifndef MIR_RENDERER_BLITTER_FRAMEBUFFER_POOL_H_
#define MIR_RENDERER_BLITTER_FRAMEBUFFER_POOL_H_

#include "common/gl_handles.h"

#include <mir/graphics/display_providers.h>
#include <mir/graphics/drm_formats.h>
#include <mir/graphics/framebuffer_ptr.h>
#include <mir/graphics/rendering_providers.h>

#include <memory>

namespace mir::renderer::blitter
{
class SoftwareEGLContext;

/// A pool of render targets, usable by both the blitter rendering path and the GL rendering
/// path.
///
/// Each pooled framebuffer is a CPU-addressable framebuffer from the display. That framebuffer's
/// dma-buf is imported into GL as an FBO-backing texture, and the same framebuffer
/// is imported as blitter Surface. This can be expensive, so framebuffers are recycled.
class FramebufferPool
{
private:
    class Entry;
    class Self;

public:
    /// Create a pool of framebuffers for \p allocator.
    ///
    /// An initial framebuffer is built eagerly, so a successfully constructed pool is known to be usable.
    ///
    /// \throws std::exception if a usable framebuffer cannot be built; the caller should
    ///         fall back to another renderer
    FramebufferPool(
        std::shared_ptr<SoftwareEGLContext> context,
        std::shared_ptr<graphics::BlitterRenderingProvider> blitter,
        graphics::CPUAddressableDisplayAllocator& allocator);

    ~FramebufferPool();

    /// A framebuffer acquired from the pool via [acquire].
    ///
    /// Owns the framebuffer until [claim_fb] hands it on. If never claimed, the framebuffer is
    /// returned to the pool on destruction.
    class PooledFB
    {
    public:
        PooledFB(PooledFB&&) noexcept;
        auto operator=(PooledFB&&) -> PooledFB& = delete;
        ~PooledFB();

        /// Make the pool's context current and bind this framebuffer as the GL draw target
        ///
        /// \pre [claim_fb] has not been called
        void bind();

        /// \pre [claim_fb] has not been called
        auto size() const -> geometry::Size;

        /// Take the blitter's handle to this framebuffer, e.g. to start a Task on it
        ///
        /// \returns the surface, or nullptr if it has already been taken and not returned
        /// \pre [claim_fb] has not been called
        auto take_surface() -> std::unique_ptr<graphics::BlitterRenderingProvider::Surface>;

        /// Give back a surface previously obtained from [take_surface]
        ///
        /// \pre [claim_fb] has not been called
        void return_surface(std::unique_ptr<graphics::BlitterRenderingProvider::Surface> surface);

        /// Hand the display's own framebuffer, as allocated, on to the display sink
        ///
        /// Dropping the returned framebuffer returns it to the pool rather than destroying it.
        /// This [PooledFB] is left empty.
        ///
        /// \pre [claim_fb] has not been called
        auto claim_fb() -> graphics::FramebufferPtr;

    private:
        friend class FramebufferPool;
        PooledFB(std::shared_ptr<Self> pool, std::unique_ptr<Entry> entry);

        std::shared_ptr<Self> pool;
        std::unique_ptr<Entry> entry;
    };

    /// Acquire a framebuffer, recycling a free one if available.
    ///
    /// \returns the acquired framebuffer
    /// \throws std::exception if a new framebuffer is needed and cannot be built
    auto acquire() -> PooledFB;

private:
    auto make_entry() -> std::unique_ptr<Entry>;

    std::shared_ptr<SoftwareEGLContext> const context;
    std::shared_ptr<graphics::BlitterRenderingProvider> const blitter;
    graphics::CPUAddressableDisplayAllocator& allocator;
    graphics::DRMFormat const format;
    std::shared_ptr<Self> self;
};

}

#endif // MIR_RENDERER_BLITTER_FRAMEBUFFER_POOL_H_
