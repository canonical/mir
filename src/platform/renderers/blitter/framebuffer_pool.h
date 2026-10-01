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
#include <mir/graphics/rendering_providers.h>

#include <memory>

namespace mir::graphics { class GLConfig; }

namespace mir::renderer::blitter
{
class SoftwareEGLContext;

using RenderbufferHandle = common::GLMultiHandle<&glDeleteRenderbuffers>;

/// A pool of render targets, usable by both the blitter and GL.
///
/// Each entry owns a CPU-adressable framebuffer from the display. That framebuffer's
/// dma-buf is imported into GL as an FBO-backing texture, and the same framebuffer
/// is imported as blitter Surface. This can be expensive, so entries are recycled.
class FramebufferPool
{
public:
    /// Create a pool of framebuffers for \p allocator.
    ///
    /// An initial [Entry] is built eagerly, so a successfully constructed pool is known to be usable.
    ///
    /// \throws std::exception if a usable framebuffer cannot be built; the caller should
    ///         fall back to another renderer
    FramebufferPool(
        std::shared_ptr<SoftwareEGLContext> context,
        std::shared_ptr<graphics::BlitterRenderingProvider> blitter,
        graphics::CPUAddressableDisplayAllocator& allocator,
        graphics::GLConfig const& config);

    ~FramebufferPool();

    class Entry
    {
    public:
        Entry(
            std::shared_ptr<SoftwareEGLContext> context,
            std::unique_ptr<graphics::CPUAddressableDisplayAllocator::MappableFB> fb,
            std::unique_ptr<graphics::BlitterRenderingProvider::Surface> surface,
            bool with_depth_stencil);
        ~Entry();

        Entry(Entry const&) = delete;
        auto operator=(Entry const&) -> Entry& = delete;

        /// Make the owning context current and bind this entry's FBO as the GL draw target
        void bind();
        auto size() const -> geometry::Size;

    private:
        std::shared_ptr<SoftwareEGLContext> const context;
        std::unique_ptr<graphics::CPUAddressableDisplayAllocator::MappableFB> const fb;
        common::TextureHandle texture;
        common::FramebufferHandle fbo;
        RenderbufferHandle depth_stencil_buffer;

    public:
        /// The blitter's handle to this framebuffer; empty while a Task holds it
        ///
        /// Declared after \ref fb so that it is destroyed before the framebuffer it targets.
        std::unique_ptr<graphics::BlitterRenderingProvider::Surface> surface;
    };

    /// A pooled framebuffer, acquired via [acquire].
    class PooledFB : public graphics::Framebuffer
    {
    public:
        virtual ~PooledFB() = default;
        virtual auto entry() -> Entry& = 0;
    };

    struct Self;

    /// Acquire a framebuffer, recycling a free one if available.
    ///
    /// The framebuffer is automatically returned to the pool when the
    /// object is destroyed.
    ///
    /// \returns the acquired framebuffer
    /// \throws std::exception if a new framebuffer is needed and cannot be built
    auto acquire() -> std::unique_ptr<PooledFB>;

private:
    auto make_entry() -> std::unique_ptr<Entry>;

    std::shared_ptr<SoftwareEGLContext> const context;
    std::shared_ptr<graphics::BlitterRenderingProvider> const blitter;
    graphics::CPUAddressableDisplayAllocator& allocator;
    graphics::DRMFormat const format;
    bool const with_depth_stencil;
    std::shared_ptr<Self> self;
};

}

#endif // MIR_RENDERER_BLITTER_FRAMEBUFFER_POOL_H_
