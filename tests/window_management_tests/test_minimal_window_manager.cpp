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

#include <miral/minimal_window_manager.h>
#include <mir/scene/session.h>
#include <mir/wayland/weak.h>
#include <mir/scene/surface_stack.h>
#include <mir/scene/surface.h>
#include <mir/executor.h>
#include <mir/events/event_builders.h>
#include <mir/events/pointer_event.h>
#include <mir/compositor/buffer_stream.h>
#include <mir/graphics/buffer_basic.h>
#include <mir/test/doubles/stub_buffer_stream.h>
#include <mir/shell/abstract_shell.h>

#include "mir_test_framework/window_management_test_harness.h"
#include <linux/input.h>

namespace ms = mir::scene;
namespace msh = mir::shell;
namespace geom = mir::geometry;
namespace mg = mir::graphics;
namespace mev = mir::events;
using namespace testing;

MATCHER_P(LockedEq, value, "")
{
    return arg.lock() == value;
}

class MinimalWindowManagerTest : public mir_test_framework::WindowManagementTestHarness
{
public:
    auto get_builder() -> mir_test_framework::WindowManagementPolicyBuilder override
    {
        return [&](miral::WindowManagerTools const& tools)
        {
            return std::make_unique<miral::MinimalWindowManager>(tools);
        };
    }

    auto get_output_rectangles() -> std::vector<mir::geometry::Rectangle> override
    {
        return {
            mir::geometry::Rectangle{{0, 0}, {800, 600}},
            mir::geometry::Rectangle{{800, 0}, {800, 600}}
        };
    }
};

TEST_F(MinimalWindowManagerTest, new_window_has_focus)
{
    auto const app = open_application("test");
    auto window = create_window(app, geom::Width{100}, geom::Height{100});
    EXPECT_EQ(focused_surface(), window.operator std::shared_ptr<ms::Surface>());
}

TEST_F(MinimalWindowManagerTest, alt_f4_closes_active_window)
{
    auto const app = open_application("test");
    auto window = create_window(app, geom::Width{100}, geom::Height{100});

    // Process an alt + f4 input
    std::chrono::nanoseconds const event_timestamp = std::chrono::system_clock::now().time_since_epoch();
    MirKeyboardAction const action{mir_keyboard_action_down};
    xkb_keysym_t const keysym{0};
    int const scan_code{KEY_F4};
    MirInputEventModifiers const modifiers{mir_input_event_modifier_alt};
    auto const event = mir::events::make_key_event(
        mir_input_event_type_key,
        event_timestamp,
        action,
        keysym,
        scan_code,
        modifiers);
    publish_event(*event);
}

TEST_F(MinimalWindowManagerTest, alt_tab_input_event_triggers_window_focus_cycling)
{
    // Create two apps, each with a single window
    auto const app1 = open_application("test");
    auto window1 = create_window(app1, geom::Width{100}, geom::Height{100});
    auto const app2 = open_application("test-1");
    auto window2 = create_window(app2, geom::Width{100}, geom::Height{100});

    EXPECT_EQ(focused_surface(), window2.operator std::shared_ptr<ms::Surface>());

    // Process an alt + tab input
    std::chrono::nanoseconds const event_timestamp = std::chrono::system_clock::now().time_since_epoch();
    MirKeyboardAction const action{mir_keyboard_action_down};
    xkb_keysym_t const keysym{0};
    int const scan_code{KEY_TAB};
    MirInputEventModifiers const modifiers{mir_input_event_modifier_alt};
    auto const event = mir::events::make_key_event(
        mir_input_event_type_key,
        event_timestamp,
        action,
        keysym,
        scan_code,
        modifiers);
    publish_event(*event);

    // Assert that the focused window is window1
    auto surface1 = window1.operator std::shared_ptr<ms::Surface>();
    auto surface2 = window2.operator std::shared_ptr<ms::Surface>();
    EXPECT_EQ(focused_surface(), surface1);
    EXPECT_THAT(
        stacking_order_of({surface1, surface2}),
        ElementsAre(LockedEq(surface2), LockedEq(surface1)));
}

TEST_F(MinimalWindowManagerTest, can_select_window_with_pointer)
{
    // Create two apps, each with a single window
    auto const app1 = open_application("test");
    auto window1 = create_window(app1, geom::Width{100}, geom::Height{100});
    auto const app2 = open_application("test-1");
    auto window2 = create_window(app2, geom::Width{50}, geom::Height{50});

    EXPECT_EQ(focused_surface(), window2.operator std::shared_ptr<ms::Surface>());

    auto const select_event = mir::events::make_pointer_event(
        0,
        std::chrono::system_clock::now().time_since_epoch(),
        mir_input_event_modifier_none,
        mir_pointer_action_button_down,
        mir_pointer_button_primary,
        geom::PointF{
            window1.top_left().x.as_int() + window1.size().width.as_int() - 1,
            window1.top_left().y.as_int() + window1.size().height.as_int() - 1},
        {},
        mir_pointer_axis_source_none,
        {},
        {});
    publish_event(*select_event);

    EXPECT_EQ(focused_surface(), window1.operator std::shared_ptr<ms::Surface>());
}

TEST_F(MinimalWindowManagerTest, can_select_window_with_touch)
{
    // Create two apps, each with a single window
    auto const app1 = open_application("test");
    auto window1 = create_window(app1, geom::Width{100}, geom::Height{100});
    auto const app2 = open_application("test-1");
    auto window2 = create_window(app2, geom::Width{50}, geom::Height{50});

    EXPECT_EQ(focused_surface(), window2.operator std::shared_ptr<ms::Surface>());

    auto const select_event = mir::events::make_touch_event(
        0,
        std::chrono::system_clock::now().time_since_epoch(),
        mir_input_event_modifier_none);
    mir::events::add_touch(
        *select_event,
        (int)std::chrono::system_clock::now().time_since_epoch().count(),
        mir_touch_action_down,
        mir_touch_tooltype_finger,
        static_cast<float>(window1.top_left().x.as_int() + window1.size().width.as_int() - 1),
        static_cast<float>(window1.top_left().y.as_int() + window1.size().height.as_int() - 1),
        1.f, 0.f, 0.f, 0.f);
    publish_event(*select_event);

    EXPECT_EQ(focused_surface(), window1.operator std::shared_ptr<ms::Surface>());
}

TEST_F(MinimalWindowManagerTest, can_move_window_with_pointer)
{
    auto const app = open_application("test");
    auto window = create_window(app, geom::Width{100}, geom::Height{100});

    // Start pointer movement
    auto initial_window_position = window.top_left();
    geom::PointF start_ptr_pos{window.top_left().x.as_int() + 50, window.top_left().y.as_int() + 50};
    geom::PointF end_ptr_pos{window.top_left().x.as_int() + 100, window.top_left().y.as_int() + 100};
    auto const start_event = mir::events::make_pointer_event(
        0,
        std::chrono::system_clock::now().time_since_epoch(),
        mir_input_event_modifier_alt,
        mir_pointer_action_button_down,
        mir_pointer_button_primary,
        start_ptr_pos,
        {},
        mir_pointer_axis_source_none,
        {},
        {});
    publish_event(*start_event);

    // Move pointer right and down
    auto const end_event = mir::events::make_pointer_event(
        0,
        std::chrono::system_clock::now().time_since_epoch(),
        mir_input_event_modifier_alt,
        mir_pointer_action_motion,
        mir_pointer_button_primary,
        end_ptr_pos,
        {},
        mir_pointer_axis_source_none,
        {},
        {});
    publish_event(*end_event);

    // Assert that the window is moved
    EXPECT_EQ(window.top_left(), geom::Point(initial_window_position.x.as_int() + 50, initial_window_position.y.as_int() + 50));
}

TEST_F(MinimalWindowManagerTest, can_resize_south_east_with_pointer)
{
    auto const app = open_application("test");
    auto window = create_window(app, geom::Width{100}, geom::Height{100});

    // Move pointer to edge of surface to simulate starting a resize from the right edge
    geom::PointF start_ptr_pos{100, 100};
    geom::PointF end_ptr_pos{50, 50};

    // Initiate the resize
    auto const start_event = mir::events::make_pointer_event(
        0,
        std::chrono::system_clock::now().time_since_epoch(),
        mir_input_event_modifier_none,
        mir_pointer_action_motion,
        mir_pointer_button_primary,
        start_ptr_pos,
        {},
        mir_pointer_axis_source_none,
        {},
        {});
    request_resize(
        window,
        start_event->to_input(),
        mir_resize_edge_southeast);

    // Move the pointer
    auto const move_event = mir::events::make_pointer_event(
        0,
        std::chrono::system_clock::now().time_since_epoch(),
        mir_input_event_modifier_none,
        mir_pointer_action_motion,
        mir_pointer_button_primary,
        end_ptr_pos,
        {},
        mir_pointer_axis_source_none,
        {},
        {});
    publish_event(*move_event);

    // Assert that the window is resized
    EXPECT_EQ(window.size(), geom::Size(50, 50));
}

TEST_F(MinimalWindowManagerTest, can_resize_south_east_with_touch)
{
    auto const app = open_application("test");
    auto window = create_window(app, geom::Width{100}, geom::Height{100});

    // Initiate the resize
    auto const start_event = mir::events::make_touch_event(
        0,
        std::chrono::system_clock::now().time_since_epoch(),
        mir_input_event_modifier_none);
    mir::events::add_touch(
        *start_event,
        (int)std::chrono::system_clock::now().time_since_epoch().count(),
        mir_touch_action_down,
        mir_touch_tooltype_finger,
        100, 100, 1.f, 0.f, 0.f, 0.f);
    request_resize(
        window,
        start_event->to_input(),
        mir_resize_edge_southeast);

    // Move the touch to (50, 50)
    auto const move_event = mir::events::make_touch_event(
        0,
        std::chrono::system_clock::now().time_since_epoch(),
        mir_input_event_modifier_none);
    mir::events::add_touch(
        *move_event,
        (int)std::chrono::system_clock::now().time_since_epoch().count(),
        mir_touch_action_change,
        mir_touch_tooltype_finger,
        50, 50, 1.f, 0.f, 0.f, 0.f);
    publish_event(*move_event);

    // Assert that the window is resized
    EXPECT_EQ(window.size(), geom::Size(50, 50));
}