/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_TEST_DOUBLES_MOCK_BLITTER_RENDERING_PROVIDER_H_
#define MIR_TEST_DOUBLES_MOCK_BLITTER_RENDERING_PROVIDER_H_

#include <gmock/gmock.h>

#include <mir/graphics/rendering_providers.h>
#include <mir/test/doubles/stub_blitter_rendering_provider_types.h>

namespace mir::test::doubles
{
class MockBlitterRenderingProvider : public graphics::BlitterRenderingProvider
{
public:
    MOCK_METHOD(graphics::probe::Result, suitability_for_display, (graphics::DisplaySink&), (override));
    MOCK_METHOD(
        graphics::probe::Result,
        suitability_for_allocator,
        (std::shared_ptr<graphics::GraphicBufferAllocator> const&),
        (override));
    MOCK_METHOD(std::unique_ptr<FramebufferProvider>, make_framebuffer_provider, (graphics::DisplaySink&), (override));
    MOCK_METHOD(std::unique_ptr<Task>, create_task, (std::unique_ptr<Surface>), (override));
    MOCK_METHOD(std::unique_ptr<Surface>, wait_complete, (std::unique_ptr<Task>), (override));
    MOCK_METHOD(
        bool,
        blit,
        (Task&,
         graphics::Buffer const&,
         geometry::Rectangle const&,
         geometry::Rectangle const&,
         MirOrientation,
         MirMirrorMode),
        (override));
    MOCK_METHOD(bool, fill, (Task&, geometry::Rectangle const&, uint8_t, uint8_t, uint8_t, uint8_t), (override));
    MOCK_METHOD(
        std::unique_ptr<Surface>,
        surface_for_fb,
        (graphics::CPUAddressableDisplayAllocator::MappableFB const&),
        (override));
    MOCK_METHOD(
        std::unique_ptr<renderer::software::Mapping<std::byte const>>,
        map_buffer,
        (graphics::Buffer const&),
        (override));
};
}

#endif // MIR_TEST_DOUBLES_MOCK_BLITTER_RENDERING_PROVIDER_H_
