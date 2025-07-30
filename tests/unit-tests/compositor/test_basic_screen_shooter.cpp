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
#include "mir/test/doubles/stub_gl_rendering_provider.h"
#include "src/server/compositor/basic_screen_shooter.h"

#include "mir/test/doubles/mock_cursor.h"
#include "mir/test/doubles/mock_scene.h"
#include "mir/test/doubles/mock_renderer.h"
#include "mir/test/doubles/mock_gl_rendering_provider.h"
#include "mir/test/doubles/mock_renderer_factory.h"
#include "mir/test/doubles/advanceable_clock.h"
#include "mir/test/doubles/explicit_executor.h"
#include "mir/test/doubles/stub_buffer.h"
#include "mir/test/doubles/stub_scene_element.h"
#include "mir/test/doubles/stub_renderable.h"
#include "mir/test/doubles/stub_gl_config.h"
#include "mir/test/doubles/stub_buffer_allocator.h"
#include "mir/test/doubles/stub_output_filter.h"

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
struct BasicScreenShooter : Test
{
    BasicScreenShooter()
    {
        ON_CALL(*scene, scene_elements_for(_)).WillByDefault(Return(scene_elements));
        ON_CALL(*renderer_factory, create_renderer_for(_,_)).WillByDefault(
                [this](auto output_surface, auto)
                {
                    ON_CALL(*next_renderer, render(_))
                        .WillByDefault(
                            [surface = std::shared_ptr<mg::gl::OutputSurface>(std::move(output_surface))]()
                            {
                                return surface->commit();
                            });

                    return std::move(next_renderer);
                });
        ON_CALL(*gl_provider, as_texture(_))
            .WillByDefault(
                [this](auto buffer)
                {
                    return default_gl_behaviour_provider.as_texture(std::move(buffer));
                });
        ON_CALL(*gl_provider, surface_for_sink(_, _))
            .WillByDefault(
                [](mg::DisplaySink& sink, auto const&)
                    -> std::unique_ptr<mg::gl::OutputSurface>
                {
                    if (auto cpu_provider = sink.acquire_compatible_allocator<mg::CPUAddressableDisplayAllocator>())
                    {
                        auto surface = std::make_unique<testing::NiceMock<mtd::MockOutputSurface>>();
                        auto format = cpu_provider->supported_formats().front();
                        ON_CALL(*surface, commit())
                            .WillByDefault(
                                [cpu_provider, format]()
                                {
                                    return cpu_provider->alloc_fb(format);
                                });
                        return surface;
                    }
                    BOOST_THROW_EXCEPTION((std::runtime_error{"CPU output support not available?!"}));
                });
        ON_CALL(*gl_provider, suitability_for_allocator(_))
            .WillByDefault(Return(mg::probe::supported));
        ON_CALL(*gl_provider, suitability_for_display(_))
            .WillByDefault(Return(mg::probe::supported));

        shooter = std::make_unique<mc::BasicScreenShooter>(
            scene,
            clock,
            executor,
            gl_providers,
            renderer_factory,
            buffer_allocator,
            std::make_shared<mtd::StubGLConfig>(),
            std::make_shared<mtd::StubOutputFilter>(),
            cursor);
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
    mtd::StubGlRenderingProvider default_gl_behaviour_provider;
    std::shared_ptr<mtd::MockGlRenderingProvider> gl_provider{std::make_shared<testing::NiceMock<mtd::MockGlRenderingProvider>>()};
    std::vector<std::shared_ptr<mg::GLRenderingProvider>> gl_providers{gl_provider};
    std::shared_ptr<mtd::MockRendererFactory> renderer_factory{std::make_shared<testing::NiceMock<mtd::MockRendererFactory>>()};
    std::shared_ptr<mtd::StubBufferAllocator> buffer_allocator;
    std::shared_ptr<mtd::AdvanceableClock> clock{std::make_shared<mtd::AdvanceableClock>()};
    std::shared_ptr<mtd::MockCursor> cursor{std::make_shared<mtd::MockCursor>()};
    mtd::ExplicitExecutor executor;
    std::unique_ptr<mc::BasicScreenShooter> shooter;
    std::shared_ptr<mtd::StubBuffer> buffer{std::make_shared<mtd::StubBuffer>(geom::Size{800, 600})};
    geom::Rectangle const viewport_rect{{20, 30}, {40, 50}};
    glm::mat2 const viewport_transform{1.f};
    StrictMock<MockFunction<void(std::optional<mir::time::Timestamp>)>> callback;
    std::optional<mir::time::Timestamp> const nullopt_time{};
};

}

TEST_F(BasicScreenShooter, calls_callback_from_executor)
{
    shooter->capture(buffer, viewport_rect, viewport_transform, false, [&](auto time)
        {
            callback.Call(time);
        });
    clock->advance_by(1s);
    EXPECT_CALL(callback, Call(std::make_optional(clock->now())));
    executor.execute();
}

TEST_F(BasicScreenShooter, renders_scene_elements)
{
    shooter->capture(buffer, viewport_rect, viewport_transform, false, [&](auto time)
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

TEST_F(BasicScreenShooter, render_curor_when_overlay_cursor_is_true)
{
    shooter->capture(buffer, viewport_rect, viewport_transform, true, [&](auto time)
        {
            callback.Call(time);
        });
    InSequence seq;
    EXPECT_CALL(*scene, scene_elements_for(_)).WillOnce(Return(scene_elements));
    auto const cursor_renderable = std::make_shared<mtd::StubRenderable>();
    EXPECT_CALL(*cursor, renderable()).WillOnce(Return(cursor_renderable));
    auto renderables_with_cursor = renderables;
    renderables_with_cursor.push_back(cursor_renderable);
    EXPECT_CALL(*next_renderer, render(Eq(renderables_with_cursor)));
    EXPECT_CALL(callback, Call(std::make_optional(clock->now())));
    executor.execute();
}

TEST_F(BasicScreenShooter, sets_viewport_correctly_before_render)
{
    shooter->capture(buffer, viewport_rect, viewport_transform, false, [&](auto time)
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
    shooter->capture(broken_buffer, viewport_rect, viewport_transform, false, [&](auto time)
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
    shooter->capture(buffer, viewport_rect, viewport_transform, false, [&](auto time)
        {
            callback.Call(time);
        });
    EXPECT_CALL(callback, Call(nullopt_time));
    executor.execute();
}

TEST_F(BasicScreenShooter, throw_in_surface_for_output_handled_gracefully)
{
    ON_CALL(*gl_provider, surface_for_sink).WillByDefault(
        [](auto&, auto&) -> std::unique_ptr<mg::gl::OutputSurface>
        {
            BOOST_THROW_EXCEPTION((std::runtime_error{"Throw in surface_for_sink"}));
        });
    shooter->capture(buffer, viewport_rect, viewport_transform, false, [&](auto time)
        {
            callback.Call(time);
        });
    EXPECT_CALL(callback, Call(nullopt_time));
    executor.execute();
}

TEST_F(BasicScreenShooter, throw_in_render_causes_graceful_failure)
{
    EXPECT_CALL(*next_renderer, render(_)).WillOnce([](auto) -> std::unique_ptr<mg::Framebuffer>
        {
            throw std::runtime_error{"throw in render()!"};
        });
    shooter->capture(buffer, viewport_rect, viewport_transform, false, [&](auto time)
        {
            callback.Call(time);
        });
    EXPECT_CALL(callback, Call(nullopt_time));
    executor.execute();
}

TEST_F(BasicScreenShooter, ensures_renderer_is_current_on_only_one_thread)
{
    thread_local bool is_current_here = false;
    std::atomic<bool> is_current = false;

    auto shooter = std::make_unique<mc::BasicScreenShooter>(
        scene,
        clock,
        mir::thread_pool_executor,
        gl_providers,
        renderer_factory,
        buffer_allocator,
        std::make_shared<mtd::StubGLConfig>(),
        std::make_shared<mtd::StubOutputFilter>(),
        cursor);

    ON_CALL(*next_renderer, render(_))
        .WillByDefault(
            [&](auto) -> std::unique_ptr<mg::Framebuffer>
            {
                /* It's OK if we're being made current again on the same thread
                 * without having been released previously.
                 * We just need to ensure that, if we are current *anywhere* then
                 * it's *this* thread that we're current on.
                 */
                EXPECT_THAT(is_current_here, Eq(is_current.load()));
                is_current = true;
                is_current_here = true;
                return {};
            });
    ON_CALL(*next_renderer, suspend())
        .WillByDefault(
            [&]()
            {
                EXPECT_THAT(is_current_here, Eq(is_current.load()));
                is_current = false;
                is_current_here = false;
            });

    // This doesn't actually have to be atomic
    std::atomic<int> call_count = 0;
    auto const spawn_a_capture =
        [&]()
        {
            shooter->capture(buffer, viewport_rect, viewport_transform, false, [&](auto) { call_count++; });
        };

    auto const expected_call_count = 20;
    for (auto i = 0; i < expected_call_count; ++i)
    {
        mir::thread_pool_executor.spawn(spawn_a_capture);
    }

    mir::ThreadPoolExecutor::quiesce();
    EXPECT_THAT(call_count, Eq(expected_call_count));
}

TEST_F(BasicScreenShooter, compositor_id_is_not_null)
{
    EXPECT_THAT(shooter->id(), NotNull());
}
