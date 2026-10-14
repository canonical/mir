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

#ifndef MIR_RENDERERS_BLITTER_RENDERER_FACTORY_H_
#define MIR_RENDERERS_BLITTER_RENDERER_FACTORY_H_

#include <mir/renderer/blitter_renderer_factory.h>

namespace mir::renderer::blitter
{

class RendererFactory : public renderer::BlitterRendererFactory
{
public:
    auto create_renderer_for(
        std::shared_ptr<graphics::BlitterRenderingProvider> blitter,
        graphics::CPUAddressableDisplayAllocator& allocator,
        std::shared_ptr<graphics::GLConfig const> config) const -> std::unique_ptr<renderer::Renderer> override;
};

}

#endif // MIR_RENDERERS_BLITTER_RENDERER_FACTORY_H_
