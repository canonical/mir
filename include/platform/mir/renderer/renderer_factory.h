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

#ifndef MIR_RENDERER_RENDERER_FACTORY_H_
#define MIR_RENDERER_RENDERER_FACTORY_H_

#include <memory>

namespace mir
{
namespace graphics
{
class GLRenderingProvider;
class BlitterRenderingProvider;
class CPUAddressableDisplayAllocator;
namespace gl { class OutputSurface; }
}
namespace renderer
{
namespace gl { class RenderTarget; }

class Renderer;

class RendererFactory
{
public:
    virtual ~RendererFactory() = default;

    virtual auto create_renderer_for(
        std::unique_ptr<graphics::gl::OutputSurface> output_surface,
        std::shared_ptr<graphics::GLRenderingProvider> gl_provider) const -> std::unique_ptr<Renderer> = 0;

protected:
    RendererFactory() = default;
    RendererFactory(RendererFactory const&) = delete;
    RendererFactory& operator=(RendererFactory const&) = delete;
};

class BlitterRendererFactory
{
public:
    virtual ~BlitterRendererFactory() = default;

    virtual auto create_renderer_for(
        graphics::CPUAddressableDisplayAllocator& allocator,
        std::shared_ptr<graphics::BlitterRenderingProvider> blitter_provider) const -> std::unique_ptr<Renderer> = 0;

protected:
    BlitterRendererFactory() = default;
    BlitterRendererFactory(BlitterRendererFactory const&) = delete;
    BlitterRendererFactory& operator=(BlitterRendererFactory const&) = delete;
};

}
}

#endif /* MIR_RENDERER_RENDERER_FACTORY_H_ */
