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

#include <mir/renderers/blitter/renderer_factory.h>
#include <mir/renderers/blitter/renderer.h>

namespace mrb = mir::renderer::blitter;

auto mrb::RendererFactory::create_renderer_for(
    std::shared_ptr<graphics::BlitterRenderingProvider> blitter,
    graphics::CPUAddressableDisplayAllocator& allocator,
    std::shared_ptr<graphics::GLConfig const> config) const -> std::unique_ptr<mir::renderer::Renderer>
{
    return std::make_unique<Renderer>(std::move(blitter), allocator, std::move(config));
}
