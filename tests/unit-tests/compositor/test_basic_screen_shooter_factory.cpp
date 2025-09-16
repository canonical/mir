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

#include "mir/executor.h"
#include "mir/graphics/platform.h"
#include "mir/graphics/display_sink.h"
#include "mir/renderer/gl/gl_surface.h"
#include "src/server/compositor/basic_screen_shooter_factory.h"

#include "mir/test/doubles/mock_scene.h"
#include "mir/test/doubles/mock_renderer.h"
#include "mir/test/doubles/mock_gl_rendering_provider.h"
#include "mir/test/doubles/mock_renderer_factory.h"
#include "mir/test/doubles/advanceable_clock.h"
#include "mir/test/doubles/explicit_executor.h"
#include "mir/test/doubles/stub_buffer.h"
#include "mir/test/doubles/stub_output_filter.h"
#include "mir/test/doubles/stub_gl_config.h"
#include "mir/test/doubles/stub_buffer_allocator.h"
#include "mir/test/doubles/stub_cursor.h"

#include <gtest/gtest.h>

namespace mc = mir::compositor;
namespace mr = mir::renderer;
namespace mg = mir::graphics;
namespace geom = mir::geometry;
namespace mtd = mir::test::doubles;
using namespace testing;

class BasicScreenShooterFactoryTest : public Test
{
public:
    BasicScreenShooterFactoryTest()
    {
        ON_CALL(*gl_provider, suitability_for_allocator(_))
            .WillByDefault(Return(mg::probe::supported));
        ON_CALL(*gl_provider, suitability_for_display(_))
            .WillByDefault(Return(mg::probe::supported));

        factory = std::make_unique<mc::BasicScreenShooterFactory>(
            scene,
            clock,
            gl_providers,
            renderer_factory,
            buffer_allocator,
            std::make_shared<mtd::StubGLConfig>(),
            std::make_shared<mtd::StubOutputFilter>(),
            std::make_shared<mtd::StubCursor>());
    }

    std::shared_ptr<mtd::MockScene> scene{std::make_shared<NiceMock<mtd::MockScene>>()};
    std::shared_ptr<mtd::AdvanceableClock> clock{std::make_shared<mtd::AdvanceableClock>()};
    mtd::ExplicitExecutor executor;
    std::shared_ptr<mtd::MockGlRenderingProvider> gl_provider{std::make_shared<NiceMock<mtd::MockGlRenderingProvider>>()};
    std::vector<std::shared_ptr<mg::GLRenderingProvider>> gl_providers{gl_provider};
    std::shared_ptr<mtd::MockRendererFactory> renderer_factory{std::make_shared<NiceMock<mtd::MockRendererFactory>>()};
    std::shared_ptr<mtd::StubBufferAllocator> buffer_allocator{std::make_shared<mtd::StubBufferAllocator>()};
    std::unique_ptr<mc::BasicScreenShooterFactory> factory;
};

TEST_F(BasicScreenShooterFactoryTest, creates_basic_screen_shooter)
{
    EXPECT_THAT(factory->create(executor), NotNull());
}
