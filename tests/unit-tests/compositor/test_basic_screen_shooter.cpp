/*
 * Copyright © 2022 Canonical Ltd.
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
 *
 * Authored by: William Wold <william.wold@canonical.com>
 */

#include "src/server/compositor/basic_screen_shooter.h"

#include "mir/renderer/renderer_factory.h"
#include "mir/renderer/gl/buffer_render_target.h"
#include "mir/test/fake_shared.h"
#include "mir/test/doubles/mock_scene.h"
#include "mir/test/doubles/null_display.h"
#include "mir/test/doubles/mock_renderer.h"
#include "mir/test/doubles/advanceable_clock.h"
#include "mir/test/doubles/explicit_executor.h"
#include "mir/test/doubles/stub_buffer.h"
#include "mir/test/doubles/stub_scene_element.h"
#include "mir/test/doubles/stub_renderable.h"

#include <gtest/gtest.h>

namespace mc = mir::compositor;
namespace mr = mir::renderer;
namespace mrg = mir::renderer::gl;
namespace mrs = mir::renderer::software;
namespace mg = mir::graphics;
namespace geom = mir::geometry;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;

using namespace testing;
using namespace std::chrono_literals;

namespace
{

class MockBufferRenderTarget: public mrg::BufferRenderTarget
{
public:
    MOCK_METHOD(void, set_buffer, (
        std::shared_ptr<mrs::WriteMappableBuffer> const& buffer,
        geom::Size const& size), (override));
    MOCK_METHOD(void, make_current, (), (override));
    MOCK_METHOD(void, release_current, (), (override));
    MOCK_METHOD(void, swap_buffers, (), (override));
    MOCK_METHOD(void, bind, (), (override));
};

struct BasicScreenShooter : Test
{
    BasicScreenShooter()
    {
        ON_CALL(scene, scene_elements_for(_)).WillByDefault(Return(scene_elements));
    }

    NiceMock<mtd::MockScene> scene;
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
    mtd::MockRenderer& renderer{*new NiceMock<mtd::MockRenderer>};
    MockBufferRenderTarget& render_target{*new NiceMock<MockBufferRenderTarget>};
    mtd::AdvanceableClock clock;
    mtd::ExplicitExecutor executor;
    mc::BasicScreenShooter shooter{
        mt::fake_shared(scene),
        mt::fake_shared(clock),
        executor,
        std::unique_ptr<mrg::BufferRenderTarget>{&render_target},
        std::unique_ptr<mr::Renderer>{&renderer}};
    mtd::StubBuffer buffer;
    geom::Rectangle const viewport_rect{{20, 30}, {40, 50}};
    StrictMock<MockFunction<void(std::optional<mir::time::Timestamp>)>> callback;
    std::optional<mir::time::Timestamp> const nullopt_time{};
};

}

TEST_F(BasicScreenShooter, calls_callback_from_executor)
{
    shooter.capture(mt::fake_shared(buffer), viewport_rect, [&](auto time)
        {
            callback.Call(time);
        });
    clock.advance_by(1s);
    EXPECT_CALL(callback, Call(std::make_optional(clock.now())));
    executor.execute();
}

TEST_F(BasicScreenShooter, renders_scene_elements)
{
    shooter.capture(mt::fake_shared(buffer), viewport_rect, [&](auto time)
        {
            callback.Call(time);
        });
    InSequence seq;
    EXPECT_CALL(scene, scene_elements_for(_)).WillOnce(Return(scene_elements));
    EXPECT_THAT(renderables.size(), Gt(0));
    EXPECT_CALL(renderer, render(Eq(renderables)));
    EXPECT_CALL(callback, Call(std::make_optional(clock.now())));
    executor.execute();
}

TEST_F(BasicScreenShooter, sets_viewport_correctly_before_render)
{
    shooter.capture(mt::fake_shared(buffer), viewport_rect, [&](auto time)
        {
            callback.Call(time);
        });
    Sequence a, b;
    EXPECT_CALL(renderer, set_output_transform(Eq(glm::mat2{1}))).InSequence(a);
    EXPECT_CALL(renderer, set_viewport(Eq(viewport_rect))).InSequence(b);
    EXPECT_CALL(renderer, render(_)).InSequence(a, b);
    EXPECT_CALL(callback, Call(std::make_optional(clock.now()))).InSequence(a, b);
    executor.execute();
}

TEST_F(BasicScreenShooter, sets_buffer_before_render)
{
    shooter.capture(mt::fake_shared(buffer), viewport_rect, [&](auto time)
        {
            callback.Call(time);
        });
    InSequence seq;
    EXPECT_CALL(render_target, make_current());
    EXPECT_CALL(render_target, set_buffer(Eq(mt::fake_shared(buffer)), Eq(viewport_rect.size)));
    EXPECT_CALL(render_target, bind());
    EXPECT_CALL(renderer, render(_));
    EXPECT_CALL(render_target, release_current());
    EXPECT_CALL(callback, Call(std::make_optional(clock.now())));
    executor.execute();
}

TEST_F(BasicScreenShooter, throw_in_scene_elements_for_causes_graceful_failure)
{
    ON_CALL(scene, scene_elements_for(_)).WillByDefault(Invoke([](auto) -> mc::SceneElementSequence
        {
            throw std::runtime_error{"throw in scene_elements_for()!"};
        }));
    shooter.capture(mt::fake_shared(buffer), viewport_rect, [&](auto time)
        {
            callback.Call(time);
        });
    EXPECT_CALL(callback, Call(nullopt_time));
    executor.execute();
}

TEST_F(BasicScreenShooter, throw_in_make_current_causes_graceful_failure)
{
    ON_CALL(render_target, make_current()).WillByDefault(Invoke([]()
        {
            throw std::runtime_error{"throw in make_current()!"};
        }));
    shooter.capture(mt::fake_shared(buffer), viewport_rect, [&](auto time)
        {
            callback.Call(time);
        });
    EXPECT_CALL(callback, Call(nullopt_time));
    executor.execute();
}

TEST_F(BasicScreenShooter, throw_in_render_causes_graceful_failure)
{
    ON_CALL(renderer, render(_)).WillByDefault(Invoke([](auto)
        {
            throw std::runtime_error{"throw in render()!"};
        }));
    shooter.capture(mt::fake_shared(buffer), viewport_rect, [&](auto time)
        {
            callback.Call(time);
        });
    EXPECT_CALL(callback, Call(nullopt_time));
    executor.execute();
}
