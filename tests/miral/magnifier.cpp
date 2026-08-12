/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
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

#include <miral/magnifier.h>
#include <mir/server.h>
#include <mir/compositor/scene.h>
#include <mir/compositor/scene_element.h>
#include <mir/graphics/display_configuration_observer.h>
#include <mir/graphics/renderable.h>
#include <mir/input/cursor_observer.h>
#include <mir/input/cursor_observer_multiplexer.h>
#include <mir/main_loop.h>
#include <mir/test/doubles/stub_display_configuration.h>
#include <mir/test/signal.h>

#include <miral/test_server.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <glm/gtc/matrix_transform.hpp>

namespace geom = mir::geometry;
namespace mi = mir::input;
namespace mtd = mir::test::doubles;

using namespace testing;
using namespace miral;
using namespace std::chrono_literals;

namespace
{
/// Raises a signal each time cursor_moved_to() fires. Used by cursor-tracking
/// tests to wait for cursor events to propagate through the main loop before
/// inspecting surface state.
struct SentinelCursorObserver : public mi::CursorObserver
{
    void cursor_moved_to(float, float) override { signal.raise(); }
    void pointer_usable() override {}
    void pointer_unusable() override {}
    void image_set_to(std::shared_ptr<mir::graphics::CursorImage>) override {}
    mir::test::Signal signal;
};

}

class MagnifierTest : public TestServer
{
public:
    MagnifierTest()
    {
        start_server_in_setup = false;
        add_to_environment("MIR_SERVER_PLATFORM_DISPLAY_LIBS", "mir:virtual");
        add_to_environment("MIR_SERVER_VIRTUAL_OUTPUT", "800x600");
    }

    void SetUp() override
    {
        TestServer::SetUp();
        add_server_init(magnifier);
    }

    auto magnifier_renderable() const -> std::shared_ptr<mir::graphics::Renderable>
    { return server().the_scene()->scene_elements_for(this).at(magnifier_index)->renderable(); }

    auto magnifier_top_left() const -> geom::Point
    { return magnifier_renderable()->screen_position().top_left; }

    auto scene_element_count() const -> size_t { return server().the_scene()->scene_elements_for(this).size(); }

    /// Waits for work already queued on the main loop to complete. The display
    /// configuration observer multiplexer dispatches on the main loop, so this
    /// is what synchronises with the magnifier's DisplayConfigObserver.
    void flush_main_loop(char const* context)
    {
        mir::test::Signal flushed;
        server().the_main_loop()->spawn([&flushed] { flushed.raise(); });
        ASSERT_TRUE(flushed.wait_for(2s)) << "timed out waiting for " << context;
    }

    void wait_for_magnifier_initialization()
    {
        flush_main_loop("magnifier initialization");
    }

    /// Registering interest replays the current cursor state, so waiting for
    /// the sentinel's first signal is how a test knows the initial placement
    /// has settled.
    void wait_for_initial_cursor_state()
    {
        auto const mux = server().the_cursor_observer_multiplexer();
        auto const sentinel = std::make_shared<SentinelCursorObserver>();
        mux->register_interest(sentinel);
        ASSERT_TRUE(sentinel->signal.wait_for(2s)) << "timed out waiting for initial cursor state";
        mux->unregister_interest(*sentinel);
    }

    /// Moves the cursor and returns once the move has been dispatched to every
    /// observer. Magnifier registers its observer from a main-loop task, so
    /// callers wanting the magnifier to have seen the move must have called
    /// wait_for_magnifier_initialization() first.
    void move_cursor_to(float x, float y)
    {
        auto const mux = server().the_cursor_observer_multiplexer();
        auto const sentinel = std::make_shared<SentinelCursorObserver>();
        mux->register_interest(sentinel);
        ASSERT_TRUE(sentinel->signal.wait_for(2s)) << "timed out waiting for initial cursor state";
        sentinel->signal.reset();

        mux->cursor_moved_to(x, y);
        ASSERT_TRUE(sentinel->signal.wait_for(2s)) << "timed out waiting for cursor event";
        mux->unregister_interest(*sentinel);
    }

    static constexpr auto magnifier_index = 0;
    Magnifier magnifier;
};

TEST_F(MagnifierTest, magnifier_disabled_by_default)
{
    add_start_callback([&]
    {
        EXPECT_THAT(scene_element_count(), Eq(0));
    });
    start_server();
}

TEST_F(MagnifierTest, can_start_enabled)
{
    magnifier.enable(true);
    add_start_callback([&]
    {
        EXPECT_THAT(scene_element_count(), Eq(1));
    });
    start_server();
}

TEST_F(MagnifierTest, magnification_results_in_scaled_transform)
{
    magnifier.magnification(2.f);
    magnifier.enable(true);
    add_start_callback([&]
    {
        EXPECT_THAT(scene_element_count(), Eq(1));
        auto const expected = glm::scale(glm::mat4(1.0), glm::vec3(2, 2, 1));
        EXPECT_THAT(magnifier_renderable()->transformation(), Eq(expected));
        EXPECT_THAT(magnifier_renderable()->screen_position().size, Eq(Size(300, 300)));
    });
    start_server();
}

TEST_F(MagnifierTest, can_set_capture_size)
{
    magnifier.capture_size(Size(200, 200));
    magnifier.enable(true);
    add_start_callback([&]
    {
        EXPECT_THAT(scene_element_count(), Eq(1));
        EXPECT_THAT(magnifier_renderable()->screen_position().size, Eq(Size(200, 200)));
    });
    start_server();
}

TEST_F(MagnifierTest, capture_size_is_limited_to_80_percent_of_the_output)
{
    magnifier.enable(true);
    start_server();

    wait_for_initial_cursor_state();

    magnifier.capture_size(Size(1000, 1000));
    magnifier_renderable()->buffer();

    auto const capture_size = magnifier_renderable()->screen_position().size;
    EXPECT_THAT(capture_size.width, Lt(Width{1000}));
    EXPECT_THAT(capture_size.height, Lt(Height{1000}));
}

TEST_F(MagnifierTest, reapplies_layout_after_outputs_are_restored)
{
    magnifier.capture_size(Size(1000, 1000)).enable(true).set_behavior(Magnifier::Behavior::freely_positioned);
    start_server();
    wait_for_magnifier_initialization();
    wait_for_initial_cursor_state();

    auto const no_outputs = std::make_shared<mtd::StubDisplayConfig>(std::vector<geom::Rectangle>{});
    server().the_display_configuration_observer()->configuration_applied(no_outputs);
    flush_main_loop("display removal");

    magnifier.capture_size(Size(1200, 1200));
    magnifier_renderable()->buffer();

    auto const restored_output = std::make_shared<mtd::StubDisplayConfig>(
        std::vector<geom::Rectangle>{{{0, 0}, {800, 600}}});
    server().the_display_configuration_observer()->configuration_applied(restored_output);
    flush_main_loop("display restoration");
    magnifier_renderable()->buffer();

    EXPECT_THAT(magnifier_renderable()->screen_position().size, Eq(Size(512, 384)));
}

// These tests run in the test body (after start_server() returns) rather than
// inside add_start_callback. Start callbacks are enqueued on the main loop via
// main_loop->enqueue, so blocking inside one waiting for more main-loop work
// would deadlock. The test body runs on a separate thread while the main loop
// runs in the background.
//
// Synchronisation: Magnifier registers its cursor observer from a main-loop
// task. Wait for that task before registering a sentinel, so the sentinel is
// ordered after Magnifier when cursor events are dispatched.

TEST_F(MagnifierTest, cursor_not_tracked_in_decoupled_mode)
{
    magnifier.enable(true).set_behavior(Magnifier::Behavior::freely_positioned);
    start_server();
    wait_for_magnifier_initialization();
    wait_for_initial_cursor_state();

    auto const before = magnifier_top_left();

    move_cursor_to(400, 300);

    EXPECT_THAT(magnifier_top_left(), Eq(before));
}

TEST_F(MagnifierTest, capture_size_change_does_not_recenter_on_cursor_in_decoupled_mode)
{
    magnifier.enable(true).set_behavior(Magnifier::Behavior::freely_positioned);
    start_server();
    wait_for_magnifier_initialization();
    wait_for_initial_cursor_state();

    // Move the cursor away from the magnifier, then re-apply the capture size
    // (as happens on a live config reload).
    move_cursor_to(700, 500);

    auto const centre = [this]
    {
        auto const rect = magnifier_renderable()->screen_position();
        return rect.top_left +
               geom::Displacement{geom::as_delta(rect.size.width / 2), geom::as_delta(rect.size.height / 2)};
    };

    auto const before = centre();
    magnifier.capture_size(Size(200, 200));
    // Claim the stale frame so the next scene read receives the resized one.
    magnifier_renderable()->buffer();
    auto const after = centre();

    EXPECT_THAT(after, Eq(before));
    EXPECT_THAT(after, Ne(geom::Point{700, 500}));
}

TEST_F(MagnifierTest, cursor_tracked_in_coupled_mode)
{
    magnifier.enable(true);
    start_server();
    wait_for_magnifier_initialization();
    wait_for_initial_cursor_state();

    auto const before = magnifier_top_left();

    move_cursor_to(400, 300);

    EXPECT_THAT(magnifier_top_left(), Ne(before));
}
