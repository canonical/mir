/*
 * Copyright © Canonical Ltd.
 *
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

#ifndef MIR_TEST_DOUBLES_STUB_BLITTER_RENDERING_PROVIDER_H_
#define MIR_TEST_DOUBLES_STUB_BLITTER_RENDERING_PROVIDER_H_

#include <mir/graphics/platform.h>
#include <mir/graphics/rendering_providers.h>
#include <mir/test/doubles/stub_blitter_rendering_provider_types.h>

namespace mir::test::doubles
{
class StubBlitterRenderingProvider : public graphics::BlitterRenderingProvider
{
public:
    auto suitability_for_display(graphics::DisplaySink&) -> graphics::probe::Result override
    {
        return graphics::probe::dummy;
    }
    auto suitability_for_allocator(std::shared_ptr<graphics::GraphicBufferAllocator> const&)
        -> graphics::probe::Result override
    {
        return graphics::probe::supported;
    }
    auto make_framebuffer_provider(graphics::DisplaySink&) -> std::unique_ptr<FramebufferProvider> override
    {
        class NullFramebufferProvider : public FramebufferProvider
        {
        public:
            auto buffer_to_framebuffer(std::shared_ptr<graphics::Buffer>)
                -> std::unique_ptr<graphics::Framebuffer> override
            {
                return {};
            }
        };
        return std::make_unique<NullFramebufferProvider>();
    }

    auto create_task(std::unique_ptr<Surface>) -> std::unique_ptr<Task> override { return nullptr; }
    auto wait_complete(std::unique_ptr<Task>) -> std::unique_ptr<Surface> override { return nullptr; }
    auto blit(
        Task&,
        graphics::Buffer const&,
        geometry::Rectangle const&,
        geometry::Rectangle const&,
        MirOrientation,
        MirMirrorMode) -> bool override
    {
        return false;
    }
    auto fill(Task&, geometry::Rectangle const&, uint8_t, uint8_t, uint8_t, uint8_t) -> bool override { return false; }
    auto surface_for_fb(graphics::CPUAddressableDisplayAllocator::MappableFB const&)
        -> std::unique_ptr<Surface> override
    {
        return nullptr;
    }
    // TODO: Remove when no longer needed in `BlitterRenderingProvider` interface.
    auto map_buffer(graphics::Buffer const&) -> std::unique_ptr<renderer::software::Mapping<std::byte const>> override
    {
        return nullptr;
    }
};
}

#endif // MIR_TEST_DOUBLES_STUB_BLITTER_RENDERING_PROVIDER_H_
