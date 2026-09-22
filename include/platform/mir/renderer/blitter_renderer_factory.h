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

#ifndef MIR_RENDERER_BLITTER_RENDERER_FACTORY_H_
#define MIR_RENDERER_BLITTER_RENDERER_FACTORY_H_

#include <memory>

namespace mir
{
namespace graphics
{
class BlitterRenderingProvider;
class CPUAddressableDisplayAllocator;
class GLConfig;
}

namespace renderer
{
class Renderer;

/**
 * Builds renderers that composite with a blitter engine.
 *
 * This is deliberately separate from renderer::RendererFactory: a blitter
 * renderer renders into the display's own buffers, so it takes the display's
 * allocator rather than a ready-made output surface.
 */
class BlitterRendererFactory
{
public:
    virtual ~BlitterRendererFactory() = default;

    /**
     * Create a renderer compositing onto `allocator`'s framebuffers with `blitter`
     *
     * \throws std::runtime_error if `blitter` and `allocator` cannot in fact be
     *         used together, in which case the caller should fall back to GL
     *         compositing.
     */
    virtual auto create_renderer_for(
        std::shared_ptr<graphics::BlitterRenderingProvider> blitter,
        graphics::CPUAddressableDisplayAllocator& allocator,
        std::shared_ptr<graphics::GLConfig const> config) const -> std::unique_ptr<Renderer> = 0;

protected:
    BlitterRendererFactory() = default;
    BlitterRendererFactory(BlitterRendererFactory const&) = delete;
    auto operator=(BlitterRendererFactory const&) -> BlitterRendererFactory& = delete;
};

}
}

#endif /* MIR_RENDERER_BLITTER_RENDERER_FACTORY_H_ */
