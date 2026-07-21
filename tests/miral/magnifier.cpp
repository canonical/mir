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
#include <mir/events/event_builders.h>
#include <mir/events/touch_contact.h>
#include <mir/graphics/renderable.h>
#include <mir/input/composite_event_filter.h>
#include <mir/input/cursor_observer.h>
#include <mir/input/cursor_observer_multiplexer.h>
#include <mir/input/event_builder.h>
#include <mir/input/input_device_hub.h>
#include <mir/input/input_device_registry.h>
#include <mir/input/input_sink.h>
#include <mir/input/virtual_input_device.h>
#include <mir/test/signal.h>
#include <mir/executor.h>
#include "add_virtual_device.h"

#include <miral/test_server.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <glm/gtc/matrix_transform.hpp>

namespace geom = mir::geometry;
namespace mi = mir::input;

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
        EXPECT_THAT(magnifier_renderable()->screen_position().size, Eq(Size(150, 150)));
    });
    start_server();
}

TEST_F(MagnifierTest, can_set_capture_size)
{
    magnifier.capture_size(Size(500, 500));
    magnifier.enable(true);
    add_start_callback([&]
    {
        EXPECT_THAT(scene_element_count(), Eq(1));
        EXPECT_THAT(magnifier_renderable()->screen_position().size, Eq(Size(500, 500)));
    });
    start_server();
}

TEST_F(MagnifierTest, decoupled_mode_shows_handle_indicators)
{
    magnifier.enable(true).stop_following_cursor();
    add_start_callback([&]
    {
        // 4 indicators + 1 magnifier surface
        EXPECT_THAT(scene_element_count(), Eq(5));
    });
    start_server();
}

TEST_F(MagnifierTest, handles_hidden_when_disabled_in_decoupled_mode)
{
    magnifier.enable(true).stop_following_cursor();
    add_start_callback([&]
    {
        magnifier.enable(false);
        EXPECT_THAT(scene_element_count(), Eq(0));
    });
    start_server();
}

TEST_F(MagnifierTest, toggling_to_coupled_hides_handles)
{
    magnifier.enable(true).stop_following_cursor();
    add_start_callback([&]
    {
        EXPECT_THAT(scene_element_count(), Eq(5));
        magnifier.follow_cursor();
        EXPECT_THAT(scene_element_count(), Eq(1));
    });
    start_server();
}

TEST_F(MagnifierTest, decoupling_after_start_shows_handles)
{
    magnifier.enable(true);
    add_start_callback([&]
    {
        EXPECT_THAT(scene_element_count(), Eq(1));
        magnifier.stop_following_cursor();
        EXPECT_THAT(scene_element_count(), Eq(5));
    });
    start_server();
}

// These tests run in the test body (after start_server() returns) rather than
// inside add_start_callback. Start callbacks are enqueued on the main loop via
// main_loop->enqueue, so blocking inside one waiting for more main-loop work
// would deadlock. The test body runs on a separate thread while the main loop
// runs in the background.
//
// Synchronisation: a SentinelCursorObserver is registered on the multiplexer.
// Because for_each_observer dispatches in registration order (magnifier's
// observer registered first), the sentinel fires only after the magnifier has
// processed each cursor event. Waiting for the sentinel's initial-state
// delivery acts as a "fence" that guarantees all earlier main-loop tasks
// (including the magnifier's own initial cursor event) have run.

TEST_F(MagnifierTest, cursor_not_tracked_in_decoupled_mode)
{
    magnifier.enable(true).stop_following_cursor();
    start_server();

    auto const mux = server().the_cursor_observer_multiplexer();
    auto sentinel = std::make_shared<SentinelCursorObserver>();
    mux->register_interest(sentinel);
    ASSERT_TRUE(sentinel->signal.wait_for(2s)) << "timed out waiting for initial cursor state";
    sentinel->signal.reset();

    auto const before = magnifier_top_left();

    mux->cursor_moved_to(400, 300);
    ASSERT_TRUE(sentinel->signal.wait_for(2s)) << "timed out waiting for cursor event";

    auto const after = magnifier_top_left();

    EXPECT_THAT(after, Eq(before));
    mux->unregister_interest(*sentinel);
}

TEST_F(MagnifierTest, cursor_tracked_in_coupled_mode)
{
    magnifier.enable(true);
    start_server();

    auto const mux = server().the_cursor_observer_multiplexer();
    auto sentinel = std::make_shared<SentinelCursorObserver>();
    mux->register_interest(sentinel);
    ASSERT_TRUE(sentinel->signal.wait_for(2s)) << "timed out waiting for initial cursor state";
    sentinel->signal.reset();

    auto const before = magnifier_top_left();

    mux->cursor_moved_to(400, 300);
    ASSERT_TRUE(sentinel->signal.wait_for(2s)) << "timed out waiting for cursor event";

    auto const after = magnifier_top_left();

    EXPECT_THAT(after, Ne(before));
    mux->unregister_interest(*sentinel);
}

// Input events are dispatched synchronously through SurfaceInputDispatcher into
// the handle surface observers, but the observers run via BasicSurface::Multiplexer
// which uses linearising_executor (deferred background thread). State changes
// (move_to, resize, set_transformation) are applied asynchronously. We flush
// the executor before reading scene state.
//
// Handle positions depend on the initial surface placement (at {0,0} from the
// BasicSurface constructor). We read positions dynamically from the live scene
// elements to avoid brittle hardcoded coordinates.
//
// Scene element ordering (fixed, determined by creation order in add_init_callback):
//   [0] magnifier surface  (size 150×150)
//   [1] drag handle        (size 48×48)
//   [2] resize handle      (size 48×48)
//   [3] zoom-in handle     (size 48×48)
//   [4] zoom-out handle    (size 48×48)

struct MagnifierHandleTest : MagnifierTest
{
    MagnifierHandleTest()
    {
        magnifier.enable(true).stop_following_cursor();

        add_server_init(
            [this](mir::Server& server)
            {
                server.add_init_callback(
                    [&server, this]
                    {
                        composite_event_filter = server.the_composite_event_filter();
                        input_device_registry = server.the_input_device_registry();
                        input_device_hub = server.the_input_device_hub();
                    });
            });
    }

    void SetUp() override
    {
        MagnifierTest::SetUp();
        start_server();
        pointer_device = miral::test::add_test_device(
            input_device_registry.lock(), input_device_hub.lock(), mi::DeviceCapability::pointer);
    }

    /// Returns the centre of the scene element at `index` as a float point.
    geom::PointF element_center(int index)
    {
        auto const pos = element_rectangle(index);
        return geom::PointF{
            static_cast<float>(pos.top_left.x.as_int() + pos.size.width.as_int() / 2.0f),
            static_cast<float>(pos.top_left.y.as_int() + pos.size.height.as_int() / 2.0f)};
    }

    geom::Rectangle element_rectangle(int index)
    {
        auto const elements = server().the_scene()->scene_elements_for(this);
        return elements.at(static_cast<std::size_t>(index))->renderable()->screen_position();
    }

    // Scene-element indices matching add_init_callback creation order.
    static constexpr auto drag_handle_index = 1;
    static constexpr auto resize_handle_index = 2;
    static constexpr auto zoom_in_handle_index = 3;
    /// centre, dragging to `target`.
    ///
    /// Input observers run on the linearising_executor (deferred background
    /// thread), so we flush it before returning to ensure surface state is
    /// fully updated when the caller checks it.
    void drag(geom::PointF from, geom::PointF to)
    {
        pointer_device->if_started_then(
            [&](mi::InputSink* sink, mi::EventBuilder* builder)
            {
                sink->handle_input(builder->pointer_event(
                    std::nullopt,
                    mir_pointer_action_button_down,
                    mir_pointer_button_primary,
                    from,
                    geom::DisplacementF{0, 0},
                    mir_pointer_axis_source_none,
                    {}, {}));
                sink->handle_input(builder->pointer_event(
                    std::nullopt,
                    mir_pointer_action_motion,
                    mir_pointer_button_primary,
                    to,
                    geom::DisplacementF{to.x.as_value() - from.x.as_value(),
                                       to.y.as_value() - from.y.as_value()},
                    mir_pointer_axis_source_none,
                    {}, {}));
                sink->handle_input(builder->pointer_event(
                    std::nullopt,
                    mir_pointer_action_button_up,
                    MirPointerButtons{0},
                    to,
                    geom::DisplacementF{0, 0},
                    mir_pointer_axis_source_none,
                    {}, {}));
            });
        flush_observer_callbacks();
    }

    /// Emit a button-down then button-up at `pos` (a tap / click).
    void click(geom::PointF pos)
    {
        pointer_device->if_started_then(
            [&](mi::InputSink* sink, mi::EventBuilder* builder)
            {
                sink->handle_input(builder->pointer_event(
                    std::nullopt,
                    mir_pointer_action_button_down,
                    mir_pointer_button_primary,
                    pos,
                    geom::DisplacementF{0, 0},
                    mir_pointer_axis_source_none,
                    {}, {}));
                sink->handle_input(builder->pointer_event(
                    std::nullopt,
                    mir_pointer_action_button_up,
                    MirPointerButtons{0},
                    pos,
                    geom::DisplacementF{0, 0},
                    mir_pointer_axis_source_none,
                    {}, {}));
            });
        flush_observer_callbacks();
    }

    // auto magnifier() const -> std::shared_ptr<mir::compositor::SceneElement>
    // { return server().the_scene()->scene_elements_for(this).at(magnifier_index); }

    std::weak_ptr<mi::InputDeviceRegistry> input_device_registry;
    std::weak_ptr<mi::InputDeviceHub> input_device_hub;
    std::weak_ptr<mi::CompositeEventFilter> composite_event_filter;
    std::shared_ptr<mi::VirtualInputDevice> pointer_device;

private:
    /// Surface input observers use BasicSurface::Multiplexer which dispatches
    /// via linearising_executor (a non-blocking deferred executor backed by a
    /// thread pool). Spawn a sentinel task AFTER the input events so that when
    /// it fires, all earlier callbacks — including the handle observer state
    /// updates (move_to, resize, set_transformation) — have already run.
    void flush_observer_callbacks()
    {
        mir::test::Signal done;
        mir::linearising_executor.spawn([&done]() { done.raise(); });
        ASSERT_TRUE(done.wait_for(2s)) << "linearising_executor timed out";
    }
};

TEST_F(MagnifierHandleTest, drag_handle_moves_magnifier)
{
    auto const before = magnifier_top_left();

    auto const from = element_center(drag_handle_index);
    drag(from, geom::PointF{from.x.as_value() + 20, from.y.as_value()});

    auto const after = magnifier_top_left();

    EXPECT_THAT(after.x.as_int(), Eq(before.x.as_int() + 20));
    EXPECT_THAT(after.y.as_int(), Eq(before.y.as_int()));
}

TEST_F(MagnifierHandleTest, drag_handle_moves_magnifier_flush_to_screen_corners)
{
    auto from = element_center(drag_handle_index);
    auto visual_top_left = element_rectangle(resize_handle_index).top_left;
    drag(
        from,
        geom::PointF{
            from.x.as_value() - visual_top_left.x.as_int(),
            from.y.as_value() - visual_top_left.y.as_int()});

    EXPECT_THAT(element_rectangle(resize_handle_index).top_left, Eq(geom::Point{0, 0}));

    from = element_center(drag_handle_index);
    auto const drag_rect = element_rectangle(drag_handle_index);
    int const visual_right = drag_rect.top_left.x.as_int() + drag_rect.size.width.as_int();
    int const visual_bottom = drag_rect.top_left.y.as_int() + drag_rect.size.height.as_int();
    drag(
        from,
        geom::PointF{
            from.x.as_value() + 800 - visual_right,
            from.y.as_value() + 600 - visual_bottom});

    auto const final_drag_rect = element_rectangle(drag_handle_index);
    EXPECT_THAT(
        final_drag_rect.top_left.x.as_int() + final_drag_rect.size.width.as_int(),
        Eq(800));
    EXPECT_THAT(
        final_drag_rect.top_left.y.as_int() + final_drag_rect.size.height.as_int(),
        Eq(600));
}

TEST_F(MagnifierHandleTest, double_clicking_drag_handle_does_not_move_magnifier)
{
    auto const before = magnifier_top_left();
    auto const handle_center = element_center(drag_handle_index);

    click(handle_center);
    click(handle_center);

    EXPECT_THAT(magnifier_top_left(), Eq(before));
}

TEST_F(MagnifierHandleTest, consumes_pointer_events_on_decoupled_magnifier_surface)
{
    auto const event = mir::events::make_pointer_event(
        MirInputDeviceId{},
        std::chrono::nanoseconds{},
        MirInputEventModifiers{},
        mir_pointer_action_motion,
        MirPointerButtons{},
        element_center(magnifier_index),
        geom::DisplacementF{},
        mir_pointer_axis_source_none,
        {},
        {});

    EXPECT_TRUE(composite_event_filter.lock()->handle(*event));
}

TEST_F(MagnifierHandleTest, consumes_touch_events_on_decoupled_magnifier_surface)
{
    auto const event = mir::events::make_touch_event(
        MirInputDeviceId{},
        std::chrono::nanoseconds{},
        MirInputEventModifiers{},
        std::vector<mir::events::TouchContact>{{
            0,
            mir_touch_action_down,
            mir_touch_tooltype_finger,
            element_center(magnifier_index),
            1.0f,
            1.0f,
            1.0f,
            0.0f}});

    EXPECT_TRUE(composite_event_filter.lock()->handle(*event));
}

TEST_F(MagnifierHandleTest, does_not_consume_events_outside_decoupled_magnifier_surface)
{
    auto const pointer_event = mir::events::make_pointer_event(
        MirInputDeviceId{},
        std::chrono::nanoseconds{},
        MirInputEventModifiers{},
        mir_pointer_action_motion,
        MirPointerButtons{},
        geom::PointF{799.0f, 599.0f},
        geom::DisplacementF{},
        mir_pointer_axis_source_none,
        {},
        {});
    auto const touch_event = mir::events::make_touch_event(
        MirInputDeviceId{},
        std::chrono::nanoseconds{},
        MirInputEventModifiers{},
        std::vector<mir::events::TouchContact>{{
            0,
            mir_touch_action_down,
            mir_touch_tooltype_finger,
            geom::PointF{799.0f, 599.0f},
            1.0f,
            1.0f,
            1.0f,
            0.0f}});

    EXPECT_FALSE(composite_event_filter.lock()->handle(*pointer_event));
    EXPECT_FALSE(composite_event_filter.lock()->handle(*touch_event));
}

TEST_F(MagnifierHandleTest, does_not_consume_events_when_magnifier_is_inactive)
{
    auto const magnifier_center = element_center(magnifier_index);
    magnifier.follow_cursor();

    auto const pointer_event = mir::events::make_pointer_event(
        MirInputDeviceId{},
        std::chrono::nanoseconds{},
        MirInputEventModifiers{},
        mir_pointer_action_motion,
        MirPointerButtons{},
        magnifier_center,
        geom::DisplacementF{},
        mir_pointer_axis_source_none,
        {},
        {});
    auto const touch_event = mir::events::make_touch_event(
        MirInputDeviceId{},
        std::chrono::nanoseconds{},
        MirInputEventModifiers{},
        std::vector<mir::events::TouchContact>{{
            0,
            mir_touch_action_down,
            mir_touch_tooltype_finger,
            magnifier_center,
            1.0f,
            1.0f,
            1.0f,
            0.0f}});

    EXPECT_FALSE(composite_event_filter.lock()->handle(*pointer_event));
    EXPECT_FALSE(composite_event_filter.lock()->handle(*touch_event));

    magnifier.stop_following_cursor().enable(false);

    EXPECT_FALSE(composite_event_filter.lock()->handle(*pointer_event));
    EXPECT_FALSE(composite_event_filter.lock()->handle(*touch_event));
}

TEST_F(MagnifierHandleTest, zoom_in_handle_increases_magnification)
{
    auto const before = magnifier_renderable()->transformation();

    click(element_center(zoom_in_handle_index));

    auto const expected = glm::scale(glm::mat4(1.0), glm::vec3(1.5f, 1.5f, 1.0f));
    EXPECT_THAT(magnifier_renderable()->transformation(), Eq(expected));
    EXPECT_THAT(magnifier_renderable()->transformation(), Ne(before));
}

// Dragging the resize handle away from the magnifier centre must increase the
// capture size, visible as the enlarged screen_position().size of the surface.
//
// screen_position().size comes from the buffer stream's TrackingSubmission. The
// MultiMonitorArbiter only advances to a newly-submitted buffer once the
// previous submission has been "claimed" (i.e. buffer() called on the
// Renderable). We trigger that manually here, mimicking what a real compositor
// render pass would do, to flush the stale pre-resize submission and expose the
// post-resize buffer with the new size.
TEST_F(MagnifierHandleTest, resize_handle_changes_capture_size)
{
    auto const mag_center = element_center(magnifier_index);
    auto const from = element_center(resize_handle_index);
    // Move 30 pixels away from the magnifier centre to enlarge the capture area.
    geom::PointF const to{
        from.x.as_value() + (from.x.as_value() >= mag_center.x.as_value() ? 30.0f : -30.0f),
        from.y.as_value() + (from.y.as_value() >= mag_center.y.as_value() ? 30.0f : -30.0f)};
    drag(from, to);

    // Advance the arbiter: claim the stale current frame so the next
    // scene_elements_for call receives the new post-resize submission.
    magnifier_renderable()->buffer();

    auto const size = magnifier_renderable()->screen_position().size;

    EXPECT_THAT(size.width.as_int(),  Gt(150));
    EXPECT_THAT(size.height.as_int(), Gt(150));
}
