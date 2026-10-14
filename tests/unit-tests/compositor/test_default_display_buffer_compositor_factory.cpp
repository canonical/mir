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

#include "src/server/compositor/default_display_buffer_compositor_factory.h"
#include "src/server/report/null_report_factory.h"

#include <mir/compositor/display_buffer_compositor.h>

#include <mir/test/doubles/mock_blitter_rendering_provider.h>
#include <mir/test/doubles/mock_blitter_renderer_factory.h>
#include <mir/test/doubles/mock_display_sink.h>
#include <mir/test/doubles/mock_gl_rendering_provider.h>
#include <mir/test/doubles/mock_renderer_factory.h>
#include <mir/test/doubles/null_gl_config.h>
#include <mir/test/doubles/stub_blitter_rendering_provider.h>
#include <mir/test/doubles/stub_buffer_allocator.h>
#include <mir/test/doubles/stub_display_sink.h>
#include <mir/test/doubles/stub_gl_rendering_provider.h>
#include <mir/test/doubles/stub_output_filter.h>
#include <mir/test/doubles/stub_renderer.h>

#include <mir/renderer/renderer_factory.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mc = mir::compositor;
namespace mg = mir::graphics;
namespace mr = mir::report;
namespace mtd = mir::test::doubles;
namespace geom = mir::geometry;

namespace
{
class StubRendererFactory : public mir::renderer::RendererFactory
{
public:
    auto create_renderer_for(std::unique_ptr<mg::gl::OutputSurface>, std::shared_ptr<mg::GLRenderingProvider>) const
        -> std::unique_ptr<mir::renderer::Renderer> override
    {
        return std::make_unique<mtd::StubRenderer>();
    }
};

class StubBlitterRendererFactory : public mir::renderer::BlitterRendererFactory
{
public:
    auto create_renderer_for(mg::CPUAddressableDisplayAllocator&, std::shared_ptr<mg::BlitterRenderingProvider>) const
        -> std::unique_ptr<mir::renderer::Renderer> override
    {
        return std::make_unique<mtd::StubRenderer>();
    }
};

struct DefaultDisplayBufferCompositorFactory : public testing::Test
{
    DefaultDisplayBufferCompositorFactory() :
        gl_providers{std::make_shared<mtd::StubGlRenderingProvider>()},
        blitter_providers{std::make_shared<mtd::StubBlitterRenderingProvider>()},
        renderer_factory{std::make_shared<StubRendererFactory>()},
        blitter_renderer_factory{std::make_shared<StubBlitterRendererFactory>()},
        buffer_allocator{std::make_shared<mtd::StubBufferAllocator>()},
        report{mr::null_compositor_report()},
        output_filter{std::make_shared<mtd::StubOutputFilter>()},
        display_sink{geom::Rectangle{{0, 0}, {640, 480}}}
    {}

    std::vector<std::shared_ptr<mg::GLRenderingProvider>> const gl_providers;
    std::vector<std::shared_ptr<mg::BlitterRenderingProvider>> const blitter_providers;
    std::shared_ptr<mir::renderer::RendererFactory> const renderer_factory;
    std::shared_ptr<mir::renderer::BlitterRendererFactory> const blitter_renderer_factory;
    std::shared_ptr<mg::GraphicBufferAllocator> const buffer_allocator;
    std::shared_ptr<mc::CompositorReport> const report;
    std::shared_ptr<mg::OutputFilter> const output_filter;
    mtd::StubDisplaySink display_sink;
};
}

TEST_F(DefaultDisplayBufferCompositorFactory, creates_compositor_when_single_gl_provider_supports_display_and_allocator)
{
    using namespace testing;

    auto supported_provider = std::make_shared<NiceMock<mtd::MockGlRenderingProvider>>();
    ON_CALL(*supported_provider, suitability_for_display(_)).WillByDefault(Return(mg::probe::supported));
    ON_CALL(*supported_provider, suitability_for_allocator(_)).WillByDefault(Return(mg::probe::supported));

    mc::DefaultDisplayBufferCompositorFactory factory{
        {supported_provider},
        blitter_providers,
        std::make_shared<mtd::NullGLConfig>(),
        renderer_factory,
        blitter_renderer_factory,
        buffer_allocator,
        report,
        output_filter};

    auto compositor = factory.create_compositor_for(display_sink);
    EXPECT_THAT(compositor, NotNull());
}

TEST_F(DefaultDisplayBufferCompositorFactory, selects_highest_suitability_gl_provider)
{
    using namespace testing;

    auto supported_provider = std::make_shared<NiceMock<mtd::MockGlRenderingProvider>>();
    auto nested_provider = std::make_shared<NiceMock<mtd::MockGlRenderingProvider>>();
    auto best_provider = std::make_shared<NiceMock<mtd::MockGlRenderingProvider>>();

    ON_CALL(*supported_provider, suitability_for_display(_)).WillByDefault(Return(mg::probe::supported));
    ON_CALL(*nested_provider, suitability_for_display(_)).WillByDefault(Return(mg::probe::nested));
    ON_CALL(*best_provider, suitability_for_display(_)).WillByDefault(Return(mg::probe::best));

    ON_CALL(*supported_provider, suitability_for_allocator(_)).WillByDefault(Return(mg::probe::supported));
    ON_CALL(*nested_provider, suitability_for_allocator(_)).WillByDefault(Return(mg::probe::supported));
    ON_CALL(*best_provider, suitability_for_allocator(_)).WillByDefault(Return(mg::probe::supported));

    auto renderer_factory = std::make_shared<StrictMock<mtd::MockRendererFactory>>();
    EXPECT_CALL(*renderer_factory, create_renderer_for(_, Eq(best_provider)))
        .WillOnce(Return(ByMove(std::make_unique<mtd::StubRenderer>())));

    mc::DefaultDisplayBufferCompositorFactory factory{
        {supported_provider, nested_provider, best_provider},
        blitter_providers,
        std::make_shared<mtd::NullGLConfig>(),
        renderer_factory,
        blitter_renderer_factory,
        buffer_allocator,
        report,
        output_filter};

    auto compositor = factory.create_compositor_for(display_sink);
    EXPECT_THAT(compositor, NotNull());
}

TEST_F(DefaultDisplayBufferCompositorFactory, does_not_select_gl_provider_with_unsupported_allocator)
{
    using namespace testing;

    auto unsupported_provider = std::make_shared<NiceMock<mtd::MockGlRenderingProvider>>();
    auto supported_provider = std::make_shared<NiceMock<mtd::MockGlRenderingProvider>>();

    ON_CALL(*unsupported_provider, suitability_for_display(_)).WillByDefault(Return(mg::probe::best));
    ON_CALL(*unsupported_provider, suitability_for_allocator(_)).WillByDefault(Return(mg::probe::unsupported));

    ON_CALL(*supported_provider, suitability_for_display(_)).WillByDefault(Return(mg::probe::supported));
    ON_CALL(*supported_provider, suitability_for_allocator(_)).WillByDefault(Return(mg::probe::supported));

    auto renderer_factory = std::make_shared<StrictMock<mtd::MockRendererFactory>>();
    EXPECT_CALL(*renderer_factory, create_renderer_for(_, Eq(supported_provider)))
        .WillOnce(Return(ByMove(std::make_unique<mtd::StubRenderer>())));

    mc::DefaultDisplayBufferCompositorFactory factory{
        {supported_provider, unsupported_provider},
        blitter_providers,
        std::make_shared<mtd::NullGLConfig>(),
        renderer_factory,
        blitter_renderer_factory,
        buffer_allocator,
        report,
        output_filter};

    auto compositor = factory.create_compositor_for(display_sink);
    EXPECT_THAT(compositor, NotNull());
}

TEST_F(DefaultDisplayBufferCompositorFactory, throws_when_no_gl_provider_is_suitable)
{
    using namespace testing;

    auto provider = std::make_shared<NiceMock<mtd::MockGlRenderingProvider>>();

    ON_CALL(*provider, suitability_for_allocator(_)).WillByDefault(Return(mg::probe::unsupported));
    ON_CALL(*provider, suitability_for_display(_)).WillByDefault(Return(mg::probe::unsupported));

    mc::DefaultDisplayBufferCompositorFactory factory{
        {
            provider,
        },
        {},
        std::make_shared<mtd::NullGLConfig>(),
        renderer_factory,
        blitter_renderer_factory,
        buffer_allocator,
        report,
        output_filter};

    EXPECT_THROW({ factory.create_compositor_for(display_sink); }, std::logic_error);
}

TEST_F(DefaultDisplayBufferCompositorFactory, throws_when_gl_provider_list_is_empty)
{
    using namespace testing;

    mc::DefaultDisplayBufferCompositorFactory factory{
        {},
        {},
        std::make_shared<mtd::NullGLConfig>(),
        renderer_factory,
        blitter_renderer_factory,
        buffer_allocator,
        report,
        output_filter};

    EXPECT_THROW({ factory.create_compositor_for(display_sink); }, std::logic_error);
}

TEST_F(DefaultDisplayBufferCompositorFactory, selects_first_provider_when_suitability_is_tied)
{
    using namespace testing;

    auto first_provider = std::make_shared<NiceMock<mtd::MockGlRenderingProvider>>();
    auto second_provider = std::make_shared<NiceMock<mtd::MockGlRenderingProvider>>();

    ON_CALL(*first_provider, suitability_for_allocator(_)).WillByDefault(Return(mg::probe::best));
    ON_CALL(*first_provider, suitability_for_display(_)).WillByDefault(Return(mg::probe::best));
    ON_CALL(*second_provider, suitability_for_allocator(_)).WillByDefault(Return(mg::probe::best));
    ON_CALL(*second_provider, suitability_for_display(_)).WillByDefault(Return(mg::probe::best));

    auto renderer_factory = std::make_shared<StrictMock<mtd::MockRendererFactory>>();
    EXPECT_CALL(*renderer_factory, create_renderer_for(_, Eq(first_provider)))
        .WillOnce(Return(ByMove(std::make_unique<mtd::StubRenderer>())));

    mc::DefaultDisplayBufferCompositorFactory factory{
        {first_provider, second_provider},
        blitter_providers,
        std::make_shared<mtd::NullGLConfig>(),
        renderer_factory,
        blitter_renderer_factory,
        buffer_allocator,
        report,
        output_filter};

    auto compositor = factory.create_compositor_for(display_sink);
    EXPECT_THAT(compositor, NotNull());
}

TEST_F(DefaultDisplayBufferCompositorFactory, prefers_blitter_provider_when_gl_provider_is_only_supported)
{
    using namespace testing;

    auto gl_provider = std::make_shared<NiceMock<mtd::MockGlRenderingProvider>>();
    ON_CALL(*gl_provider, suitability_for_allocator(_)).WillByDefault(Return(mg::probe::supported));
    ON_CALL(*gl_provider, suitability_for_display(_)).WillByDefault(Return(mg::probe::supported));

    auto blitter_provider = std::make_shared<NiceMock<mtd::MockBlitterRenderingProvider>>();
    ON_CALL(*blitter_provider, suitability_for_display(_)).WillByDefault(Return(mg::probe::best));
    ON_CALL(*blitter_provider, suitability_for_allocator(_)).WillByDefault(Return(mg::probe::supported));

    auto blitter_renderer_factory = std::make_shared<StrictMock<mtd::MockBlitterRendererFactory>>();
    EXPECT_CALL(*blitter_renderer_factory, create_renderer_for(_, Eq(blitter_provider)))
        .WillOnce(Return(ByMove(std::make_unique<mtd::StubRenderer>())));

    mc::DefaultDisplayBufferCompositorFactory factory{
        {gl_provider},
        {blitter_provider},
        std::make_shared<mtd::NullGLConfig>(),
        renderer_factory,
        blitter_renderer_factory,
        buffer_allocator,
        report,
        output_filter};

    auto compositor = factory.create_compositor_for(display_sink);
    EXPECT_THAT(compositor, NotNull());
}

TEST_F(DefaultDisplayBufferCompositorFactory, prefers_blitter_provider_when_gl_provider_is_unsupported)
{
    using namespace testing;

    auto gl_provider = std::make_shared<NiceMock<mtd::MockGlRenderingProvider>>();
    ON_CALL(*gl_provider, suitability_for_allocator(_)).WillByDefault(Return(mg::probe::unsupported));
    ON_CALL(*gl_provider, suitability_for_display(_)).WillByDefault(Return(mg::probe::unsupported));

    auto blitter_provider = std::make_shared<NiceMock<mtd::MockBlitterRenderingProvider>>();
    ON_CALL(*blitter_provider, suitability_for_display(_)).WillByDefault(Return(mg::probe::best));
    ON_CALL(*blitter_provider, suitability_for_allocator(_)).WillByDefault(Return(mg::probe::supported));

    auto blitter_renderer_factory = std::make_shared<StrictMock<mtd::MockBlitterRendererFactory>>();
    EXPECT_CALL(*blitter_renderer_factory, create_renderer_for(_, Eq(blitter_provider)))
        .WillOnce(Return(ByMove(std::make_unique<mtd::StubRenderer>())));

    mc::DefaultDisplayBufferCompositorFactory factory{
        {gl_provider},
        {blitter_provider},
        std::make_shared<mtd::NullGLConfig>(),
        renderer_factory,
        blitter_renderer_factory,
        buffer_allocator,
        report,
        output_filter};

    auto compositor = factory.create_compositor_for(display_sink);
    EXPECT_THAT(compositor, NotNull());
}

TEST_F(DefaultDisplayBufferCompositorFactory, prefers_gl_provider_when_both_supported)
{
    using namespace testing;

    auto gl_provider = std::make_shared<NiceMock<mtd::MockGlRenderingProvider>>();
    ON_CALL(*gl_provider, suitability_for_allocator(_)).WillByDefault(Return(mg::probe::supported));
    ON_CALL(*gl_provider, suitability_for_display(_)).WillByDefault(Return(mg::probe::supported));

    auto blitter_provider = std::make_shared<NiceMock<mtd::MockBlitterRenderingProvider>>();
    ON_CALL(*blitter_provider, suitability_for_display(_)).WillByDefault(Return(mg::probe::supported));
    ON_CALL(*blitter_provider, suitability_for_allocator(_)).WillByDefault(Return(mg::probe::supported));

    auto renderer_factory = std::make_shared<StrictMock<mtd::MockRendererFactory>>();
    EXPECT_CALL(*renderer_factory, create_renderer_for(_, Eq(gl_provider)))
        .WillOnce(Return(ByMove(std::make_unique<mtd::StubRenderer>())));

    mc::DefaultDisplayBufferCompositorFactory factory{
        {gl_provider},
        {blitter_provider},
        std::make_shared<mtd::NullGLConfig>(),
        renderer_factory,
        blitter_renderer_factory,
        buffer_allocator,
        report,
        output_filter};

    auto compositor = factory.create_compositor_for(display_sink);
    EXPECT_THAT(compositor, NotNull());
}

TEST_F(DefaultDisplayBufferCompositorFactory, ignore_blitter_providers_when_gl_best)
{
    using namespace testing;

    auto gl_provider = std::make_shared<NiceMock<mtd::MockGlRenderingProvider>>();
    ON_CALL(*gl_provider, suitability_for_allocator(_)).WillByDefault(Return(mg::probe::best));
    ON_CALL(*gl_provider, suitability_for_display(_)).WillByDefault(Return(mg::probe::best));

    auto blitter_provider = std::make_shared<StrictMock<mtd::MockBlitterRenderingProvider>>();

    auto renderer_factory = std::make_shared<StrictMock<mtd::MockRendererFactory>>();
    EXPECT_CALL(*renderer_factory, create_renderer_for(_, Eq(gl_provider)))
        .WillOnce(Return(ByMove(std::make_unique<mtd::StubRenderer>())));

    mc::DefaultDisplayBufferCompositorFactory factory{
        {gl_provider},
        {blitter_provider},
        std::make_shared<mtd::NullGLConfig>(),
        renderer_factory,
        blitter_renderer_factory,
        buffer_allocator,
        report,
        output_filter};

    auto compositor = factory.create_compositor_for(display_sink);
    EXPECT_THAT(compositor, NotNull());
}

TEST_F(DefaultDisplayBufferCompositorFactory, ignore_blitter_providers_incompatible_display_sink)
{
    using namespace testing;

    auto gl_provider = std::make_shared<NiceMock<mtd::MockGlRenderingProvider>>();
    ON_CALL(*gl_provider, suitability_for_allocator(_)).WillByDefault(Return(mg::probe::supported));
    ON_CALL(*gl_provider, suitability_for_display(_)).WillByDefault(Return(mg::probe::supported));

    auto blitter_provider = std::make_shared<StrictMock<mtd::MockBlitterRenderingProvider>>();

    NiceMock<mtd::MockDisplaySink> display_sink{};
    ON_CALL(display_sink, maybe_create_allocator(_)).WillByDefault(Return(nullptr));

    auto renderer_factory = std::make_shared<StrictMock<mtd::MockRendererFactory>>();
    EXPECT_CALL(*renderer_factory, create_renderer_for(_, Eq(gl_provider)))
        .WillOnce(Return(ByMove(std::make_unique<mtd::StubRenderer>())));

    mc::DefaultDisplayBufferCompositorFactory factory{
        {gl_provider},
        {blitter_provider},
        std::make_shared<mtd::NullGLConfig>(),
        renderer_factory,
        blitter_renderer_factory,
        buffer_allocator,
        report,
        output_filter};

    auto compositor = factory.create_compositor_for(display_sink);
    EXPECT_THAT(compositor, NotNull());
}

TEST_F(DefaultDisplayBufferCompositorFactory, does_not_select_blitter_provider_with_unsupported_allocator)
{
    using namespace testing;

    auto supported = std::make_shared<NiceMock<mtd::MockBlitterRenderingProvider>>();
    auto unsupported = std::make_shared<NiceMock<mtd::MockBlitterRenderingProvider>>();

    ON_CALL(*supported, suitability_for_display(_)).WillByDefault(Return(mg::probe::supported));
    ON_CALL(*supported, suitability_for_allocator(_)).WillByDefault(Return(mg::probe::supported));

    ON_CALL(*unsupported, suitability_for_display(_)).WillByDefault(Return(mg::probe::best));
    ON_CALL(*unsupported, suitability_for_allocator(_)).WillByDefault(Return(mg::probe::unsupported));

    auto blitter_renderer_factory = std::make_shared<StrictMock<mtd::MockBlitterRendererFactory>>();
    EXPECT_CALL(*blitter_renderer_factory, create_renderer_for(_, Eq(supported)))
        .WillOnce(Return(ByMove(std::make_unique<mtd::StubRenderer>())));

    mc::DefaultDisplayBufferCompositorFactory factory{
        {},
        {supported, unsupported},
        std::make_shared<mtd::NullGLConfig>(),
        renderer_factory,
        blitter_renderer_factory,
        buffer_allocator,
        report,
        output_filter};

    auto compositor = factory.create_compositor_for(display_sink);
    EXPECT_THAT(compositor, NotNull());
}
