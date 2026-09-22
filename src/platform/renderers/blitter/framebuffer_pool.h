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
#include <mutex>
#include <vector>

namespace mir::graphics
{
class GLConfig;
}

namespace mir::renderer::blitter
{
class SoftwareEGLContext;

using RenderbufferHandle = common::GLMultiHandle<&glDeleteRenderbuffers>;

/**
 * A pool of render targets usable by both the blitter engine and GL.
 *
 * Each entry owns a CPU-addressable framebuffer from the display, that
 * framebuffer's dma-buf imported into GL as an FBO-backing texture, and the
 * same framebuffer imported as a blitter Surface. Building all of that is
 * expensive, so entries are recycled: `acquire()` hands out a Framebuffer that
 * returns its entry to the pool once the display has finished with it.
 */
class FramebufferPool : public std::enable_shared_from_this<FramebufferPool>
{
public:
    /**
     * Create a pool of framebuffers for `allocator`
     *
     * This eagerly builds one entry, so that failure to render into the
     * display's framebuffers is detected before we commit to this renderer.
     *
     * \throws std::runtime_error if the allocator's framebuffers cannot be used
     *         as a target by both GL and the blitter.
     */
    static auto create(
        std::shared_ptr<SoftwareEGLContext> context,
        std::shared_ptr<graphics::BlitterRenderingProvider> blitter,
        graphics::CPUAddressableDisplayAllocator& allocator,
        graphics::GLConfig const& config) -> std::shared_ptr<FramebufferPool>;

    ~FramebufferPool();

    /// A framebuffer that both GL and the blitter can render into
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
        void bind_gl();

        auto size() const -> geometry::Size;

        /// The blitter's handle to this framebuffer; empty while a Task holds it
        std::unique_ptr<graphics::BlitterRenderingProvider::Surface> surface;

    private:
        std::shared_ptr<SoftwareEGLContext> const context;
        std::unique_ptr<graphics::CPUAddressableDisplayAllocator::MappableFB> const fb;
        common::TextureHandle texture;
        common::FramebufferHandle fbo;
        RenderbufferHandle depth_stencil_buffer;
    };

    /// A framebuffer checked out of the pool
    struct Checkout
    {
        /// Valid until `framebuffer` is destroyed
        Entry& entry;
        /// The handle to hand to the display
        std::unique_ptr<graphics::Framebuffer> framebuffer;
    };

    /**
     * Check a framebuffer out of the pool, creating one if none are free
     *
     * \throws std::runtime_error on failure to build a new entry
     */
    auto acquire() -> Checkout;

    /// The size of the framebuffers in this pool
    auto size() const -> geometry::Size;

private:
    FramebufferPool(
        std::shared_ptr<SoftwareEGLContext> context,
        std::shared_ptr<graphics::BlitterRenderingProvider> blitter,
        graphics::CPUAddressableDisplayAllocator& allocator,
        graphics::DRMFormat format,
        bool with_depth_stencil);

    class PooledFB;

    auto make_entry() -> std::unique_ptr<Entry>;
    void release(std::unique_ptr<Entry> entry);

    std::shared_ptr<SoftwareEGLContext> const context;
    std::shared_ptr<graphics::BlitterRenderingProvider> const blitter;
    graphics::CPUAddressableDisplayAllocator& allocator;
    graphics::DRMFormat const format;
    bool const with_depth_stencil;
    geometry::Size const output_size;
    std::mutex mutable free_entries_mutex;
    std::vector<std::unique_ptr<Entry>> free_entries;
};

}

#endif // MIR_RENDERER_BLITTER_FRAMEBUFFER_POOL_H_
