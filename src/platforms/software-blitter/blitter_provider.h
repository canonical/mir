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

#ifndef MIR_GRAPHICS_SOFTWARE_BLITTER_PROVIDER_H
#define MIR_GRAPHICS_SOFTWARE_BLITTER_PROVIDER_H

#include <mir/geometry/rectangle.h>
#include <mir/graphics/platform.h>
#include <mir/graphics/rendering_providers.h>
#include <mir/renderer/sw/pixel_source.h>

#include <memory>

namespace mir
{
namespace graphics
{
namespace software_blitter
{

class SoftwareBlitterRenderingProvider : public graphics::BlitterRenderingProvider
{
public:
    SoftwareBlitterRenderingProvider();

    auto create_task(std::unique_ptr<Surface> surf) -> std::unique_ptr<Task> override;

    auto wait_complete(std::unique_ptr<Task> task) -> std::unique_ptr<Surface> override;

    auto blit(
        Task& task,
        Buffer const& source,
        geometry::Rectangle const& source_rect,
        geometry::Rectangle const& target_rect,
        MirOrientation rotation,
        MirMirrorMode mirror_mode) -> bool override;

    auto fill(Task& task, geometry::Rectangle const& target_rect, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
        -> bool override;

    auto surface_for_fb(CPUAddressableDisplayAllocator::MappableFB const& fb) -> std::unique_ptr<Surface> override;

    auto map_buffer(Buffer const& buffer) -> std::unique_ptr<renderer::software::Mapping<std::byte const>> override;

    auto suitability_for_display(DisplaySink& sink) -> probe::Result override;

    auto suitability_for_allocator(std::shared_ptr<GraphicBufferAllocator> const& target) -> probe::Result override;

    auto make_framebuffer_provider(DisplaySink& sink) -> std::unique_ptr<FramebufferProvider> override;
};

}
}
}

#endif // MIR_GRAPHICS_SOFTWARE_BLITTER_PROVIDER_H
