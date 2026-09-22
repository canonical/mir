/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_RENDERER_BLITTER_RENDERER_H_
#define MIR_RENDERER_BLITTER_RENDERER_H_

#include <mir/renderer/renderer.h>

#include <memory>

namespace mir
{
namespace graphics
{
class BlitterRenderingProvider;
class CPUAddressableDisplayAllocator;
class GLConfig;
}

namespace renderer::blitter
{

/**
 * A renderer that composites with a 2D blit engine.
 *
 * Rather than texturing each renderable with the GPU, this walks the scene
 * back-to-front handing each renderable to a BlitterRenderingProvider. This is
 * a win on embedded hardware that has a blitter but little or no useful GPU.
 *
 * A blitter is not a general-purpose compositor, though: it will refuse
 * operations it cannot express. Whenever that happens this falls back to
 * drawing that one renderable with OpenGL — through a software rasteriser, on
 * the assumption that a platform choosing the blitter has no GPU worth using.
 * Interleaving the two requires flushing the blitter's queue and then blocking
 * on the GL draw, so a scene where many renderables are rejected will be slower
 * than plain GL compositing.
 *
 * \note Unlike gl::Renderer this owns its rendering target: it renders directly
 *       into the display's CPU-addressable scanout buffers, which is what makes
 *       them reachable by both the blitter and GL.
 */
class Renderer : public renderer::Renderer
{
public:
    /**
     * \param [in] blitter    The provider to composite with
     * \param [in] allocator  Source of the display's scanout buffers
     * \param [in] config     Depth/stencil requirements of the GL fallback
     *
     * \throws std::runtime_error if the blitter and GL cannot share the
     *         display's framebuffers, in which case the caller should use a
     *         gl::Renderer instead.
     */
    Renderer(
        std::shared_ptr<graphics::BlitterRenderingProvider> blitter,
        graphics::CPUAddressableDisplayAllocator& allocator,
        std::shared_ptr<graphics::GLConfig const> config);
    ~Renderer() override;

    void set_viewport(geometry::Rectangle const& rect) override;
    void set_output_transform(glm::mat2 const&) override;
    void set_output_filter(MirOutputFilter filter) override;
    auto render(graphics::RenderableList const&) const -> std::unique_ptr<graphics::Framebuffer> override;
    void suspend() override;

private:
    class Impl;
    std::unique_ptr<Impl> const impl;
};

}
}

#endif // MIR_RENDERER_BLITTER_RENDERER_H_
