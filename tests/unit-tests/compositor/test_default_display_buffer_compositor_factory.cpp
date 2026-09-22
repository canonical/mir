/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "src/server/compositor/default_display_buffer_compositor_factory.h"
#include "src/server/report/null_report_factory.h"

#include <mir/compositor/display_buffer_compositor.h>
#include <mir/graphics/display_sink.h>
#include <mir/graphics/output_filter.h>
#include <mir/graphics/platform.h>
#include <mir/graphics/rendering_providers.h>
#include <mir/renderer/blitter_renderer_factory.h>
#include <mir/test/doubles/mock_renderer_factory.h>
#include <mir/test/doubles/null_display_sink.h>
#include <mir/test/doubles/stub_buffer_allocator.h>
#include <mir/test/doubles/stub_display_sink.h>
#include <mir/test/doubles/stub_gl_config.h>
#include <mir/test/doubles/stub_gl_rendering_provider.h>
#include <mir/test/doubles/stub_output_filter.h>
#include <mir/test/doubles/stub_renderer.h>

#include <stdexcept>

namespace geom = mir::geometry;
namespace mc = mir::compositor;
namespace mg = mir::graphics;
namespace mtd = mir::test::doubles;

namespace
{
class StubBlitterRenderingProvider : public mg::BlitterRenderingProvider
{
public:
    explicit StubBlitterRenderingProvider(mg::probe::Result suitability)
        : suitability{suitability}
    {
    }

    auto suitability_for_display(mg::DisplaySink&) -> mg::probe::Result override
    {
        return suitability;
    }

    auto suitability_for_allocator(std::shared_ptr<mg::GraphicBufferAllocator> const&)
        -> mg::probe::Result override
    {
        return suitability;
    }

    auto make_framebuffer_provider(mg::DisplaySink&) -> std::unique_ptr<FramebufferProvider> override
    {
        class NullFramebufferProvider : public FramebufferProvider
        {
        public:
            auto buffer_to_framebuffer(std::shared_ptr<mg::Buffer>) -> std::unique_ptr<mg::Framebuffer> override
            {
                return {};
            }
        };
        return std::make_unique<NullFramebufferProvider>();
    }

    auto create_task(std::unique_ptr<Surface>) -> std::unique_ptr<Task> override
    {
        return {};
    }

    auto wait_complete(std::unique_ptr<Task>) -> std::unique_ptr<Surface> override
    {
        return {};
    }

    auto blit(Task&, mg::Buffer const&, geom::Rectangle const&, geom::Rectangle const&, MirOrientation, MirMirrorMode)
        -> bool override
    {
        return true;
    }

    auto fill(Task&, geom::Rectangle const&, uint8_t, uint8_t, uint8_t, uint8_t) -> bool override
    {
        return true;
    }

    auto surface_for_fb(mg::CPUAddressableDisplayAllocator::MappableFB const&) -> std::unique_ptr<Surface> override
    {
        return {};
    }

    auto map_buffer(mg::Buffer const&) -> std::unique_ptr<mir::renderer::software::Mapping<std::byte const>> override
    {
        return {};
    }

private:
    mg::probe::Result const suitability;
};

class MockBlitterRendererFactory : public mir::renderer::BlitterRendererFactory
{
public:
    MockBlitterRendererFactory()
    {
        using namespace testing;
        ON_CALL(*this, create_renderer_for(_, _, _))
            .WillByDefault([](auto&&...) { return std::make_unique<mtd::StubRenderer>(); });
    }

    MOCK_METHOD(
        std::unique_ptr<mir::renderer::Renderer>,
        create_renderer_for,
        (std::shared_ptr<mg::BlitterRenderingProvider>,
         mg::CPUAddressableDisplayAllocator&,
         std::shared_ptr<mg::GLConfig const>),
        (const, override));
};

/// A display that cannot hand out CPU-addressable framebuffers
class SinkWithoutCPUAllocator : public mtd::NullDisplaySink
{
public:
    auto view_area() const -> geom::Rectangle override
    {
        return {{0, 0}, {1280, 1024}};
    }

    auto maybe_create_allocator(mg::DisplayAllocator::Tag const&) -> mg::DisplayAllocator* override
    {
        return nullptr;
    }
};

struct DefaultDisplayBufferCompositorFactory : testing::Test
{
    auto factory_with(std::vector<std::shared_ptr<mg::BlitterRenderingProvider>> blitters)
        -> mc::DefaultDisplayBufferCompositorFactory
    {
        return mc::DefaultDisplayBufferCompositorFactory{
            {gl_provider},
            std::move(blitters),
            std::make_shared<mtd::StubGLConfig>(),
            gl_renderer_factory,
            blitter_renderer_factory,
            buffer_allocator,
            mir::report::null_compositor_report(),
            std::make_shared<mtd::StubOutputFilter>()};
    }

    std::shared_ptr<mtd::StubGlRenderingProvider> const gl_provider{
        std::make_shared<mtd::StubGlRenderingProvider>()};
    std::shared_ptr<testing::NiceMock<mtd::MockRendererFactory>> const gl_renderer_factory{
        std::make_shared<testing::NiceMock<mtd::MockRendererFactory>>()};
    std::shared_ptr<testing::NiceMock<MockBlitterRendererFactory>> const blitter_renderer_factory{
        std::make_shared<testing::NiceMock<MockBlitterRendererFactory>>()};
    std::shared_ptr<mg::GraphicBufferAllocator> const buffer_allocator{
        std::make_shared<mtd::StubBufferAllocator>()};
    mtd::StubDisplaySink sink{geom::Rectangle{{0, 0}, {1280, 1024}}};
};
}

TEST_F(DefaultDisplayBufferCompositorFactory, composites_with_gl_when_no_blitter_is_available)
{
    using namespace testing;

    EXPECT_CALL(*blitter_renderer_factory, create_renderer_for(_, _, _)).Times(0);
    EXPECT_CALL(*gl_renderer_factory, create_renderer_for(_, _))
        .WillOnce([](auto&&...) { return std::make_unique<mtd::StubRenderer>(); });

    auto factory = factory_with({});
    EXPECT_THAT(factory.create_compositor_for(sink), NotNull());
}

TEST_F(DefaultDisplayBufferCompositorFactory, composites_with_gl_when_no_blitter_supports_the_output)
{
    using namespace testing;

    EXPECT_CALL(*blitter_renderer_factory, create_renderer_for(_, _, _)).Times(0);
    EXPECT_CALL(*gl_renderer_factory, create_renderer_for(_, _))
        .WillOnce([](auto&&...) { return std::make_unique<mtd::StubRenderer>(); });

    auto factory = factory_with({std::make_shared<StubBlitterRenderingProvider>(mg::probe::unsupported)});
    EXPECT_THAT(factory.create_compositor_for(sink), NotNull());
}

TEST_F(DefaultDisplayBufferCompositorFactory, composites_with_a_suitable_blitter)
{
    using namespace testing;

    auto const blitter = std::make_shared<StubBlitterRenderingProvider>(mg::probe::supported);

    EXPECT_CALL(*blitter_renderer_factory, create_renderer_for(Eq(blitter), Ref(*sink.acquire_compatible_allocator<mg::CPUAddressableDisplayAllocator>()), _));
    EXPECT_CALL(*gl_renderer_factory, create_renderer_for(_, _)).Times(0);

    auto factory = factory_with({blitter});
    EXPECT_THAT(factory.create_compositor_for(sink), NotNull());
}

TEST_F(DefaultDisplayBufferCompositorFactory, picks_the_best_of_several_blitters)
{
    using namespace testing;

    auto const worse = std::make_shared<StubBlitterRenderingProvider>(mg::probe::supported);
    auto const better = std::make_shared<StubBlitterRenderingProvider>(mg::probe::best);

    EXPECT_CALL(*blitter_renderer_factory, create_renderer_for(Eq(better), _, _));

    auto factory = factory_with({worse, better});
    EXPECT_THAT(factory.create_compositor_for(sink), NotNull());
}

TEST_F(DefaultDisplayBufferCompositorFactory, composites_with_gl_when_the_display_has_no_cpu_addressable_allocator)
{
    using namespace testing;

    SinkWithoutCPUAllocator gl_only_sink;

    EXPECT_CALL(*blitter_renderer_factory, create_renderer_for(_, _, _)).Times(0);
    EXPECT_CALL(*gl_renderer_factory, create_renderer_for(_, _))
        .WillOnce([](auto&&...) { return std::make_unique<mtd::StubRenderer>(); });

    auto factory = factory_with({std::make_shared<StubBlitterRenderingProvider>(mg::probe::supported)});
    EXPECT_THAT(factory.create_compositor_for(gl_only_sink), NotNull());
}

TEST_F(DefaultDisplayBufferCompositorFactory, falls_back_to_gl_when_the_blitter_renderer_cannot_be_built)
{
    using namespace testing;

    EXPECT_CALL(*blitter_renderer_factory, create_renderer_for(_, _, _))
        .WillOnce(Throw(std::runtime_error{"No llvmpipe for you"}));
    EXPECT_CALL(*gl_renderer_factory, create_renderer_for(_, _))
        .WillOnce([](auto&&...) { return std::make_unique<mtd::StubRenderer>(); });

    auto factory = factory_with({std::make_shared<StubBlitterRenderingProvider>(mg::probe::supported)});
    EXPECT_THAT(factory.create_compositor_for(sink), NotNull());
}
