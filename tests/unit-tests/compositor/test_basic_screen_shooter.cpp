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

#include "mir/graphics/platform.h"
#include "mir/renderer/gl/gl_surface.h"
#include "mir/test/doubles/stub_gl_rendering_provider.h"
#include "src/server/compositor/basic_screen_shooter.h"

#include "mir/renderer/renderer_factory.h"
#include "mir/test/doubles/mock_scene.h"
#include "mir/test/doubles/mock_renderer.h"
#include "mir/test/doubles/mock_gl_rendering_provider.h"
#include "mir/test/doubles/stub_gl_rendering_provider.h"
#include "mir/test/doubles/advanceable_clock.h"
#include "mir/test/doubles/explicit_executor.h"
#include "mir/test/doubles/stub_buffer.h"
#include "mir/test/doubles/stub_scene_element.h"
#include "mir/test/doubles/stub_renderable.h"

#include <gtest/gtest.h>

namespace mc = mir::compositor;
namespace mr = mir::renderer;
namespace mg = mir::graphics;
namespace geom = mir::geometry;
namespace mtd = mir::test::doubles;

using namespace testing;
using namespace std::chrono_literals;

namespace
{
class MockRendererFactory : public mr::RendererFactory
{
public:
    MOCK_METHOD(
        std::unique_ptr<mr::Renderer>,
        create_renderer_for,
        (std::unique_ptr<mg::gl::OutputSurface>, std::shared_ptr<mg::GLRenderingProvider>),
        (const override));
};

struct BasicScreenShooter : Test
{
    BasicScreenShooter()
    {
        ON_CALL(*scene, scene_elements_for(_)).WillByDefault(Return(scene_elements));
        ON_CALL(*renderer_factory, create_renderer_for(_,_)).WillByDefault(
            InvokeWithoutArgs(
                [this]()
                {
                    return std::move(next_renderer);
                }));
        ON_CALL(*gl_provider, as_texture(_))
            .WillByDefault(
                [this](auto buffer)
                {
                    return default_gl_behaviour_provider.as_texture(std::move(buffer));
                });
        ON_CALL(*gl_provider, surface_for_output(_, _, _))
            .WillByDefault(
                [this](std::shared_ptr<mg::DisplayInterfaceProvider> provider, auto size, mg::GLConfig const& config)
                {
                    return default_gl_behaviour_provider.surface_for_output(std::move(provider), size, config);
                });
        ON_CALL(*gl_provider, suitability_for_display(_))
            .WillByDefault(Return(mg::probe::supported));

        shooter = std::make_unique<mc::BasicScreenShooter>(
            scene,
            clock,
            executor,
            gl_providers,
            renderer_factory);
    }

    std::unique_ptr<mtd::MockRenderer> next_renderer{std::make_unique<testing::NiceMock<mtd::MockRenderer>>()};

    std::shared_ptr<mtd::MockScene> scene{std::make_shared<NiceMock<mtd::MockScene>>()};
    mg::RenderableList renderables{[]()
        {
            mg::RenderableList renderables;
            for (int i = 0; i < 6; i++)
            {
                renderables.push_back(std::make_shared<mtd::StubRenderable>());
            }
            return renderables;
        }()};
    mc::SceneElementSequence scene_elements{[&]()
        {
            mc::SceneElementSequence elements;
            for (auto& renderable : renderables)
            {
                elements.push_back(std::make_shared<mtd::StubSceneElement>(renderable));
            }
            return elements;
        }()};
    mtd::StubGlRenderingPlatform default_gl_behaviour_provider;
    std::shared_ptr<mtd::MockGlRenderingPlatform> gl_provider{std::make_shared<testing::NiceMock<mtd::MockGlRenderingPlatform>>()};
    std::vector<std::shared_ptr<mg::GLRenderingProvider>> gl_providers{gl_provider};
    std::shared_ptr<MockRendererFactory> renderer_factory{std::make_shared<testing::NiceMock<MockRendererFactory>>()};
    std::shared_ptr<mtd::AdvanceableClock> clock{std::make_shared<mtd::AdvanceableClock>()};
    mtd::ExplicitExecutor executor;
    std::unique_ptr<mc::BasicScreenShooter> shooter;
    std::shared_ptr<mtd::StubBuffer> buffer{std::make_shared<mtd::StubBuffer>(geom::Size{800, 600})};
    geom::Rectangle const viewport_rect{{20, 30}, {40, 50}};
    StrictMock<MockFunction<void(std::optional<mir::time::Timestamp>)>> callback;
    std::optional<mir::time::Timestamp> const nullopt_time{};
};

}

TEST_F(BasicScreenShooter, calls_callback_from_executor)
{
    shooter->capture(buffer, viewport_rect, [&](auto time)
        {
            callback.Call(time);
        });
    clock->advance_by(1s);
    EXPECT_CALL(callback, Call(std::make_optional(clock->now())));
    executor.execute();
}

TEST_F(BasicScreenShooter, renders_scene_elements)
{
    shooter->capture(buffer, viewport_rect, [&](auto time)
        {
            callback.Call(time);
        });
    InSequence seq;
    EXPECT_CALL(*scene, scene_elements_for(_)).WillOnce(Return(scene_elements));
    EXPECT_THAT(renderables.size(), Gt(0));
    EXPECT_CALL(*next_renderer, render(Eq(renderables)));
    EXPECT_CALL(callback, Call(std::make_optional(clock->now())));
    executor.execute();
}

TEST_F(BasicScreenShooter, sets_viewport_correctly_before_render)
{
    shooter->capture(buffer, viewport_rect, [&](auto time)
        {
            callback.Call(time);
        });
    Sequence a, b;
    EXPECT_CALL(*next_renderer, set_viewport(Eq(viewport_rect))).InSequence(b);
    EXPECT_CALL(*next_renderer, render(_)).InSequence(a, b);
    EXPECT_CALL(callback, Call(std::make_optional(clock->now()))).InSequence(a, b);
    executor.execute();
}

TEST_F(BasicScreenShooter, graceful_failure_on_zero_sized_buffer)
{
    auto broken_buffer = std::make_shared<mtd::StubBuffer>(geom::Size{0, 0});
    shooter->capture(broken_buffer, viewport_rect, [&](auto time)
        {
            callback.Call(time);
        });
    EXPECT_CALL(callback, Call(nullopt_time));
    executor.execute();
}

TEST_F(BasicScreenShooter, throw_in_scene_elements_for_causes_graceful_failure)
{
    ON_CALL(*scene, scene_elements_for(_)).WillByDefault(Invoke([](auto) -> mc::SceneElementSequence
        {
            throw std::runtime_error{"throw in scene_elements_for()!"};
        }));
    shooter->capture(buffer, viewport_rect, [&](auto time)
        {
            callback.Call(time);
        });
    EXPECT_CALL(callback, Call(nullopt_time));
    executor.execute();
}

TEST_F(BasicScreenShooter, throw_in_surface_for_output_handled_gracefully)
{
    ON_CALL(*gl_provider, surface_for_output).WillByDefault(
        [](auto, auto, auto&) -> std::unique_ptr<mg::gl::OutputSurface>
        {
            BOOST_THROW_EXCEPTION((std::runtime_error{"Throw in surface_for_output"}));
        });
    shooter->capture(buffer, viewport_rect, [&](auto time)
        {
            callback.Call(time);
        });
    EXPECT_CALL(callback, Call(nullopt_time));
    executor.execute();
}

TEST_F(BasicScreenShooter, throw_in_render_causes_graceful_failure)
{
    ON_CALL(*next_renderer, render(_)).WillByDefault(Invoke([](auto) -> std::unique_ptr<mg::Framebuffer>
        {
            throw std::runtime_error{"throw in render()!"};
        }));
    shooter->capture(buffer, viewport_rect, [&](auto time)
        {
            callback.Call(time);
        });
    EXPECT_CALL(callback, Call(nullopt_time));
    executor.execute();
}
