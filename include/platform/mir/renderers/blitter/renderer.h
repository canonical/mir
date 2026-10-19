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
}

namespace renderer::blitter
{

/// A renderer that composites with a 2D blit engine.
///
/// Rather than texturing each renderable with the GPU, this walks the scene
/// back-to-front handing each renderable to a [BlitterRenderingProvider]. This is
/// a win on embedded hardware that has a blitter but little or no useful GPU.
///
/// It falls back to rendering with GL in cases where blitting fails.
class Renderer : public renderer::Renderer
{
public:
    /// \param blitter    The provider to composite with
    /// \param allocator  Source of the display's scanout buffers
    /// \param config     Depth/stencil requirements of the GL fallback
    ///
    /// \throws std::exception if the blitter and GL cannot share the
    ///         display's framebuffers, in which case the caller should use a
    ///         gl::Renderer instead.
    Renderer(
        std::shared_ptr<graphics::BlitterRenderingProvider> blitter,
        graphics::CPUAddressableDisplayAllocator& allocator);
    ~Renderer() override;

    void set_viewport(geometry::Rectangle const& rect) override;
    void set_output_transform(glm::mat2 const&) override;
    void set_output_filter(MirOutputFilter filter) override;
    auto render(graphics::RenderableList const&) const -> graphics::FramebufferPtr override;
    void suspend() override;

private:
    class Impl;
    std::unique_ptr<Impl> const impl;
};

}
}

#endif // MIR_RENDERER_BLITTER_RENDERER_H_
