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

#define MIR_LOG_COMPONENT "test_minimal_window_manager"

#include "mir/geometry/forward.h"
#include "mir_test_framework/window_management_test_harness.h"
#include <miral/minimal_window_manager.h>
#include <miral/output.h>
#include <mir/scene/session.h>
#include <mir/wayland/weak.h>
#include <mir/scene/surface.h>
#include <mir/executor.h>
#include <mir/events/event_builders.h>
#include <mir/events/pointer_event.h>
#include <mir/compositor/buffer_stream.h>
#include <mir/log.h>
#include <mir/graphics/default_display_configuration_policy.h>
#include <mir/shell/shell.h>
#include <linux/input.h>
#include <mir/shell/shell.h>

#include <gtest/gtest.h>
#include <gtest/gtest-spi.h>
#include <gmock/gmock.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace ms = mir::scene;
namespace msh = mir::shell;
namespace geom = mir::geometry;
namespace mg = mir::graphics;
namespace mev = mir::events;
using namespace testing;

namespace
{
constexpr int exclusive_surface_size = 50;
typedef std::function<miral::WindowSpecification(geom::Size const& output_size)> CreateSurfaceSpecFunc;

CreateSurfaceSpecFunc create_top_attached()
{
    return [](geom::Size const& output_size)
    {
        miral::WindowSpecification spec;
        spec.size() = geom::Size(output_size.width, geom::Height{exclusive_surface_size});
        spec.top_left() = geom::Point{0, 0};
        spec.exclusive_rect() = geom::Rectangle{
            geom::Point{0, 0},
            spec.size().value()
        };
        spec.attached_edges() = static_cast<MirPlacementGravity>(mir_placement_gravity_north | mir_placement_gravity_east | mir_placement_gravity_west);
        spec.state() = mir_window_state_attached;
        spec.depth_layer() = mir_depth_layer_application;
        return spec;
    };
}

CreateSurfaceSpecFunc create_left_attached()
{
    return [](geom::Size const& output_size)
    {
        miral::WindowSpecification spec;
        spec.size() = { geom::Width{exclusive_surface_size}, output_size.height };
        spec.top_left() = geom::Point{0, 0};
        spec.exclusive_rect() = geom::Rectangle{
            geom::Point{0, 0},
            spec.size().value()
        };
        spec.attached_edges() = static_cast<MirPlacementGravity>(mir_placement_gravity_north | mir_placement_gravity_south | mir_placement_gravity_west);
        spec.state() = mir_window_state_attached;
        spec.depth_layer() = mir_depth_layer_application;
        return spec;
    };
}

CreateSurfaceSpecFunc create_right_attached()
{
    return [](geom::Size const& output_size)
    {
        miral::WindowSpecification spec;
        spec.size() = { geom::Width{exclusive_surface_size}, output_size.height };
        spec.top_left() = geom::Point{output_size.width.as_int() - spec.size().value().width.as_int(), 0};
        spec.exclusive_rect() = geom::Rectangle{
            geom::Point{0, 0},
            spec.size().value()
        };
        spec.attached_edges() = static_cast<MirPlacementGravity>(mir_placement_gravity_north | mir_placement_gravity_south | mir_placement_gravity_east);
        spec.state() = mir_window_state_attached;
        spec.depth_layer() = mir_depth_layer_application;
        return spec;
    };
}

CreateSurfaceSpecFunc create_bottom_attached()
{
    return [](geom::Size const& output_size)
    {
        miral::WindowSpecification spec;
        spec.size() = { output_size.width, geom::Height{exclusive_surface_size} };
        spec.top_left() = geom::Point{0, output_size.height.as_int() - spec.size().value().height.as_int()};
        spec.exclusive_rect() = geom::Rectangle{
            geom::Point{0, 0},
            spec.size().value()
        };
        spec.attached_edges() = static_cast<MirPlacementGravity>(mir_placement_gravity_south | mir_placement_gravity_east | mir_placement_gravity_west);
        spec.state() = mir_window_state_attached;
        spec.depth_layer() = mir_depth_layer_application;
        return spec;
    };
}
}

class MinimalWindowManagerTest : public mir_test_framework::WindowManagementTestHarness
{
public:
    MinimalWindowManagerTest() : WindowManagementTestHarness()
    {
        server.wrap_display_configuration_policy([&](std::shared_ptr<mg::DisplayConfigurationPolicy> const&)
            -> std::shared_ptr<mg::DisplayConfigurationPolicy>
        {
            return std::make_shared<mir::graphics::SideBySideDisplayConfigurationPolicy>();
        });
    }

    auto get_builder() -> mir_test_framework::WindowManagementPolicyBuilder override
    {
        return [&](miral::WindowManagerTools const& tools)
        {
            return std::make_unique<miral::MinimalWindowManager>(tools, focus_stealing());
        };
    }

    auto get_initial_output_configs() -> std::vector<mir::graphics::DisplayConfigurationOutput> override
    {
        return output_configs_from_output_rectangles({
            mir::geometry::Rectangle{{0, 0}, {800, 600}},
            mir::geometry::Rectangle{{800, 0}, {1000, 600}}
        });
    }

    virtual miral::FocusStealing focus_stealing() const
    {
        return miral::FocusStealing::allow;
    }
};

TEST_F(MinimalWindowManagerTest, new_window_has_focus)
{
    auto const app = open_application("test");
    miral::WindowSpecification spec;
    spec.size() = { geom::Width {100}, geom::Height{100} };
    spec.depth_layer() = mir_depth_layer_application;
    auto window = create_window(app, spec);
    EXPECT_TRUE(focused(window));
}

TEST_F(MinimalWindowManagerTest, alt_f4_closes_active_window)
{
    auto const app = open_application("test");
    miral::WindowSpecification spec;
    spec.size() = { geom::Width {100}, geom::Height{100} };
    spec.depth_layer() = mir_depth_layer_application;
    auto window = create_window(app, spec);
    EXPECT_TRUE(focused(window));

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

    EXPECT_TRUE(focused(miral::Window()));
}

TEST_F(MinimalWindowManagerTest, can_navigate_between_apps_using_alt_tab)
{
    // Create two apps, each with a single window
    auto const app1 = open_application("test");
    miral::WindowSpecification spec1;
    spec1.size() = { geom::Width {100}, geom::Height{100} };
    spec1.depth_layer() = mir_depth_layer_application;
    auto window1 = create_window(app1, spec1);
    auto const app2 = open_application("test-1");
    miral::WindowSpecification spec2;
    spec2.size() = { geom::Width {100}, geom::Height{100} };
    spec2.depth_layer() = mir_depth_layer_application;
    auto window2 = create_window(app2, spec2);

    EXPECT_TRUE(focused(window2));

    // Process an alt + tab input
    auto const alt_tab_down_event = mir::events::make_key_event(
        mir_input_event_type_key,
        std::chrono::system_clock::now().time_since_epoch(),
        mir_keyboard_action_down,
        0,
        KEY_TAB,
        mir_input_event_modifier_alt);
    publish_event(*alt_tab_down_event);

    // Assert that the focused window is window1
    EXPECT_TRUE(focused(window1));

    // Finish an alt + tab input
    auto const alt_tab_upevent = mir::events::make_key_event(
        mir_input_event_type_key,
        std::chrono::system_clock::now().time_since_epoch(),
        mir_keyboard_action_down,
        0,
        KEY_LEFTALT,
        mir_input_event_modifier_none);
    publish_event(*alt_tab_upevent);

    // Assert that the focused window is window1
    EXPECT_TRUE(focused(window1));
}

TEST_F(MinimalWindowManagerTest, can_navigate_within_an_app_using_alt_grave)
{
    // Create two windows on the same app
    auto const app1 = open_application("test");
    miral::WindowSpecification spec1;
    spec1.size() = { geom::Width {100}, geom::Height{100} };
    spec1.depth_layer() = mir_depth_layer_application;
    auto window1 = create_window(app1, spec1);

    miral::WindowSpecification spec2;
    spec2.size() = { geom::Width {100}, geom::Height{100} };
    spec2.depth_layer() = mir_depth_layer_application;
    auto window2 = create_window(app1, spec2);

    EXPECT_TRUE(focused(window2));

    // Process an alt + grave input
    auto const alt_grave_down_event = mir::events::make_key_event(
        mir_input_event_type_key,
        std::chrono::system_clock::now().time_since_epoch(),
        mir_keyboard_action_down,
        0,
        KEY_GRAVE,
        mir_input_event_modifier_alt);
    publish_event(*alt_grave_down_event);

    // Assert that the focused window is window1
    EXPECT_TRUE(focused(window1));

    // Finish an alt + grave input
    auto const alt_grave_upevent = mir::events::make_key_event(
        mir_input_event_type_key,
        std::chrono::system_clock::now().time_since_epoch(),
        mir_keyboard_action_down,
        0,
        KEY_GRAVE,
        mir_input_event_modifier_none);
    publish_event(*alt_grave_upevent);

    // Assert that the focused window is window1
    EXPECT_TRUE(focused(window1));
}

TEST_F(MinimalWindowManagerTest, can_select_window_with_pointer)
{
    // Create two apps, each with a single window
    auto const app1 = open_application("test");
    miral::WindowSpecification spec1;
    spec1.size() = { geom::Width {100}, geom::Height{100} };
    spec1.depth_layer() = mir_depth_layer_application;
    auto window1 = create_window(app1, spec1);
    auto const app2 = open_application("test-1");
    miral::WindowSpecification spec2;
    spec2.size() = { geom::Width {50}, geom::Height{50} };
    spec2.depth_layer() = mir_depth_layer_application;
    auto window2 = create_window(app2, spec2);

    EXPECT_TRUE(focused(window2));

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

    EXPECT_TRUE(focused(window1));
}

TEST_F(MinimalWindowManagerTest, can_select_window_with_touch)
{
    // Create two apps, each with a single window
    auto const app1 = open_application("test");
    miral::WindowSpecification spec1;
    spec1.size() = { geom::Width {100}, geom::Height{100} };
    spec1.depth_layer() = mir_depth_layer_application;
    auto window1 = create_window(app1, spec1);
    miral::WindowSpecification spec2;
    spec2.size() = { geom::Width {50}, geom::Height{50} };
    spec2.depth_layer() = mir_depth_layer_application;
    auto window2 = create_window(app1, spec2);

    EXPECT_TRUE(focused(window2));

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

    EXPECT_TRUE(focused(window1));
}

TEST_F(MinimalWindowManagerTest, can_move_window_with_pointer)
{
    auto const app = open_application("test");
    miral::WindowSpecification spec;
    spec.size() = { geom::Width {100}, geom::Height{100} };
    spec.depth_layer() = mir_depth_layer_application;
    auto window = create_window(app, spec);

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

TEST_F(MinimalWindowManagerTest, can_move_window_with_pointer_even_when_maximized)
{
    auto const app = open_application("test");
    miral::WindowSpecification spec;
    spec.state() = mir_window_state_maximized;
    spec.depth_layer() = mir_depth_layer_application;
    spec.size() = { geom::Width {100}, geom::Height{100} };
    spec.top_left() = geom::Point{0, 0};
    auto window = create_window(app, spec);

    // Start pointer movement
    auto initial_window_position = window.top_left();
    geom::PointF start_ptr_pos{window.top_left().x.as_int() + 0, window.top_left().y.as_int() + 0};
    geom::PointF end_ptr_pos{window.top_left().x.as_int() + 50, window.top_left().y.as_int() + 50};
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

    auto& info = tools().info_for(window);
    EXPECT_EQ(info.state(), mir_window_state_restored);
}

TEST_F(MinimalWindowManagerTest, can_move_window_with_touch)
{
    mir::log_info("can_move_window_with_touch: opening application");
    auto const app = open_application("test");
    miral::WindowSpecification spec;
    spec.size() = { geom::Width {100}, geom::Height{100} };
    spec.depth_layer() = mir_depth_layer_application;

    mir::log_info("can_move_window_with_touch: creating window");
    auto window = create_window(app, spec);

    // Start pointer movement
    auto initial_window_position = window.top_left();
    geom::PointF start_ptr_pos{window.top_left().x.as_int() + 50, window.top_left().y.as_int() + 50};
    geom::PointF end_ptr_pos{window.top_left().x.as_int() + 100, window.top_left().y.as_int() + 100};

    // Start movement
    auto const start_event = mir::events::make_touch_event(
        0,
        std::chrono::system_clock::now().time_since_epoch(),
        mir_input_event_modifier_shift);
    mir::events::add_touch(
        *start_event,
        (int)std::chrono::system_clock::now().time_since_epoch().count(),
        mir_touch_action_down,
        mir_touch_tooltype_finger,
        start_ptr_pos.x.as_value(), start_ptr_pos.y.as_value(), 1.f, 0.f, 0.f, 0.f);

    mir::log_info("can_move_window_with_touch: requesting move");
    request_move(window, start_event->to_input());
    mir::log_info("can_move_window_with_touch: move request complete");

    // Move pointer right and down
    auto const move_event = mir::events::make_touch_event(
        0,
        std::chrono::system_clock::now().time_since_epoch(),
        mir_input_event_modifier_shift);
    mir::events::add_touch(
        *move_event,
        (int)std::chrono::system_clock::now().time_since_epoch().count(),
        mir_touch_action_change,
        mir_touch_tooltype_finger,
        end_ptr_pos.x.as_value(), end_ptr_pos.y.as_value(), 1.f, 0.f, 0.f, 0.f);

    mir::log_info("can_move_window_with_touch: publishing event");
    publish_event(*move_event);
    mir::log_info("can_move_window_with_touch: event published");

    // Assert that the window is moved
    EXPECT_EQ(window.top_left(), geom::Point(initial_window_position.x.as_int() + 50, initial_window_position.y.as_int() + 50));
}

TEST_F(MinimalWindowManagerTest, can_resize_south_east_with_pointer)
{
    auto const app = open_application("test");
    miral::WindowSpecification spec;
    spec.size() = { geom::Width {100}, geom::Height{100} };
    spec.depth_layer() = mir_depth_layer_application;
    auto window = create_window(app, spec);

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
    miral::WindowSpecification spec;
    spec.size() = { geom::Width {100}, geom::Height{100} };
    spec.depth_layer() = mir_depth_layer_application;
    auto window = create_window(app, spec);

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

template <uint N>
class MultipleWindowsMinimalWindowManagerTest : public MinimalWindowManagerTest
{
public:
    void SetUp() override
    {
        MinimalWindowManagerTest::SetUp();
        miral::WindowSpecification spec;
        spec.size() = { geom::Width {100}, geom::Height{100} };
        spec.depth_layer() = mir_depth_layer_application;

        for (uint i = 0; i < N; i++)
        {
            apps[i] = open_application("app" + std::to_string(i));
            windows[i] = create_window(apps[i], spec);
        }
    }

    miral::Application apps[N];
    miral::Window windows[N];
};

template <uint N>
class MultipleWindowsMinimalWindowManagerPreventFocusStealingTest : public MultipleWindowsMinimalWindowManagerTest<N>
{
public:
    miral::FocusStealing focus_stealing() const override
    {
        return miral::FocusStealing::prevent;
    }
};

using MinimalWindowManagerWithTwoWindows = MultipleWindowsMinimalWindowManagerTest<2>;
using MinimalWindowManagerWithThreeWindows = MultipleWindowsMinimalWindowManagerTest<3>;
using MinimalWindowManagerWithThreeWindowsPreventFocusStealing = MultipleWindowsMinimalWindowManagerPreventFocusStealingTest<3>;

TEST_F(MinimalWindowManagerWithThreeWindows, new_windows_are_inserted_above_older_windows)
{
    EXPECT_TRUE(is_above(windows[2], windows[1]));
    EXPECT_TRUE(is_above(windows[1], windows[0]));
}

TEST_F(MinimalWindowManagerWithThreeWindows, when_a_window_is_focused_then_it_appears_above_other_windows)
{
    tools().select_active_window(windows[0]);
    EXPECT_TRUE(focused(windows[0]));
    EXPECT_TRUE(is_above(windows[0], windows[2]));
    EXPECT_TRUE(is_above(windows[2], windows[1]));
}

TEST_F(MinimalWindowManagerWithThreeWindowsPreventFocusStealing,
    when_a_window_is_opened_with_prevent_focus_stealing_then_it_does_not_appears_above_other_windows)
{
    EXPECT_TRUE(focused(windows[0]));
    EXPECT_TRUE(is_above(windows[0], windows[2]));
    EXPECT_TRUE(is_above(windows[2], windows[1]));
}

TEST_F(MinimalWindowManagerWithTwoWindows, if_active_window_is_removed_then_parent_has_focus_priority)
{
    miral::WindowSpecification spec;
    spec.size() = { geom::Width {100}, geom::Height{100} };
    spec.parent() = windows[0];
    auto const child_window = create_window(apps[0], spec);

    tools().ask_client_to_close(child_window);
    EXPECT_TRUE(focused(windows[0]));
    EXPECT_TRUE(is_above(windows[0], windows[1]));
}

TEST_F(MinimalWindowManagerWithThreeWindows, if_active_window_is_removed_then_last_focused_window_is_focused)
{
    tools().ask_client_to_close(windows[2]);
    EXPECT_TRUE(focused(windows[1]));
    EXPECT_TRUE(is_above(windows[1], windows[0]));
}

TEST_F(MinimalWindowManagerWithThreeWindows, when_active_window_is_closed_then_we_focus_a_window_in_same_workspaces)
{
    auto const workspace = tools().create_workspace();
    tools().add_tree_to_workspace(windows[0], workspace);
    tools().add_tree_to_workspace(windows[2], workspace);

    tools().ask_client_to_close(windows[2]);
    EXPECT_TRUE(focused(windows[0]));
    EXPECT_TRUE(is_above(windows[0], windows[1]));
}

TEST_F(MinimalWindowManagerWithTwoWindows,
    if_active_window_is_removed_and_we_cannot_focus_any_window_on_closing_window_workspaces_then_try_focus_within_app)
{
    auto const workspace = tools().create_workspace();
    miral::WindowSpecification spec;
    spec.size() = { geom::Width {100}, geom::Height{100} };
    auto const window_on_first_app = create_window(apps[0], spec);
    tools().add_tree_to_workspace(window_on_first_app, workspace);

    tools().ask_client_to_close(window_on_first_app);
    EXPECT_TRUE(focused(windows[0]));
    EXPECT_TRUE(is_above(windows[0], windows[1]));
}

TEST_F(MinimalWindowManagerWithThreeWindows,
    if_active_window_is_removed_and_we_cannot_focus_any_window_on_closing_window_workspaces_and_there_are_no_other_windows_in_this_app_then_focus_other_app)
{
    auto const workspace1 = tools().create_workspace();
    tools().add_tree_to_workspace(windows[1], workspace1);
    auto const workspace2 = tools().create_workspace();
    tools().add_tree_to_workspace(windows[2], workspace2);

    tools().ask_client_to_close(windows[2]);
    EXPECT_TRUE(focused(windows[1]));
    EXPECT_TRUE(is_above(windows[1], windows[0]));
}

TEST_F(MinimalWindowManagerTest, if_active_window_is_removed_and_it_is_only_window_then_nothing_is_focused)
{
    // Setup: Create one app with one window
    auto const app = open_application("app1");
    miral::WindowSpecification spec;
    spec.size() = { geom::Width {100}, geom::Height{100} };
    spec.depth_layer() = mir_depth_layer_application;
    auto const window = create_window(app, spec);

    EXPECT_TRUE(focused(window));

    // Act: Close the focused window
    tools().ask_client_to_close(window);

    // Expect: no window to have focus
    EXPECT_TRUE(focused({}));
}

TEST_F(MinimalWindowManagerTest,
    if_active_windows_session_closes_and_there_is_nothing_to_select_then_focused_is_empty)
{
    // Setup: Create one app with one window
    auto const app = open_application("app1");
    miral::WindowSpecification spec;
    spec.size() = { geom::Width {100}, geom::Height{100} };
    spec.depth_layer() = mir_depth_layer_application;
    auto const window = create_window(app, spec);

    // Act: Close the session
    server.the_shell()->close_session(app);

    // Expect: no window to have focus
    EXPECT_TRUE(focused({}));
}

TEST_F(MinimalWindowManagerTest, closing_attached_window_causes_maximized_to_resize)
{
    // Setup: Create one app with a maximized window and one app with a top attached window
    auto const output_rectangle =  get_initial_output_configs()[0];
    auto const app = open_application("app1");
    miral::WindowSpecification spec;
    spec.size() = { geom::Width {100}, geom::Height{100} };
    spec.depth_layer() = mir_depth_layer_application;
    spec.state() = mir_window_state_maximized;
    auto const window = create_window(app, spec);

    auto const top_spec = create_top_attached();
    auto const attached = create_window(app, top_spec(output_rectangle.extents().size));
    EXPECT_THAT(window.size(), Eq(geom::Size(
        output_rectangle.extents().size.width.as_int(),
        output_rectangle.extents().size.height.as_int() - exclusive_surface_size
    )));

    // Act: Close the attached window
    tools().ask_client_to_close(attached);

    // Expect: the window now takes up the full dimensions
    EXPECT_THAT(window.size(), Eq(geom::Size(
        output_rectangle.extents().size.width.as_int(),
        output_rectangle.extents().size.height.as_int()
    )));
}

class MinimalWindowManagerStartMoveStateChangeTest
    : public MinimalWindowManagerTest,
      public ::testing::WithParamInterface<MirWindowState>
{
};

TEST_P(MinimalWindowManagerStartMoveStateChangeTest, maximized_windows_that_are_moved_get_restored)
{
    auto const state = GetParam();

    // Open a regular window at point (200, 200) with size (100, 100)
    // We do not start in the maximized state so that the restore_rect of the
    // window has been calculated properly.
    auto const app = open_application("app");
    miral::WindowSpecification spec;
    spec.size() = { geom::Width {100}, geom::Height{100} };
    spec.top_left() = geom::Point{200, 200};
    spec.depth_layer() = mir_depth_layer_application;
    spec.state() = mir_window_state_restored;
    auto const window = create_window(app, spec);

    // Then maximize the window.
    miral::WindowSpecification new_spec;
    new_spec.state() = state;
    tools().modify_window(window, new_spec);

    // Next, start the movement
    int constexpr pointer_x = 150;
    int constexpr pointer_y = 150;
    auto const start_event = mir::events::make_touch_event(
        0,
        std::chrono::system_clock::now().time_since_epoch(),
        mir_input_event_modifier_shift);
    mir::events::add_touch(
        *start_event,
        static_cast<int>(std::chrono::system_clock::now().time_since_epoch().count()),
        mir_touch_action_down,
        mir_touch_tooltype_finger,
        pointer_x, pointer_y, 1.f, 0.f, 0.f, 0.f);

    // Finally, assert that we are in the maximized state before the window is moved,
    // and that we exit the maximized state afterward.
    auto const& info = tools().info_for(window);
    ASSERT_EQ(info.state(), state);
    request_move(window, start_event->to_input());
    ASSERT_EQ(info.state(), mir_window_state_restored);
    ASSERT_EQ(window.top_left(), geom::Point(pointer_x, pointer_y));
    ASSERT_EQ(window.size(), geom::Size(spec.size().value().width, spec.size().value().height));
}

INSTANTIATE_TEST_SUITE_P(
    MinimalWindowManagerStartMoveStateChangeTest,
    MinimalWindowManagerStartMoveStateChangeTest,
    ::testing::Values(
        mir_window_state_maximized,
        mir_window_state_vertmaximized,
        mir_window_state_horizmaximized,
        mir_window_state_attached)
);

/// Represents a collection of attached surfaces (i.e. wlr layer shell surfaces)
/// that will be placed on the first output to confirm how they are placed
/// relative to one another.
struct AttachedSurfacePlacementCase
{
    struct AttachedSurfacePlacement
    {
        CreateSurfaceSpecFunc create;
        std::function<geom::Rectangle(geom::Size const&)> expected_rect;
    };

    std::string name;
    std::vector<AttachedSurfacePlacement> placements;
};

struct AttachedSurfacePlacementCaseToString {
    std::string operator()(const TestParamInfo<AttachedSurfacePlacementCase>& info) const {
        return info.param.name;
    }
};

class MinimalWindowManagerAttachedTest
    : public MinimalWindowManagerTest,
      public ::testing::WithParamInterface<AttachedSurfacePlacementCase>
{
};

TEST_P(MinimalWindowManagerAttachedTest, attached_window_positioning)
{
    auto const app = open_application("test");
    auto const& placements = GetParam().placements;
    auto const output_rectangles = get_initial_output_configs();

    std::vector<miral::Window> windows;
    auto const& output_rectangle = output_rectangles[0];
    for (const auto& placement : placements)
        windows.push_back(create_window(app, placement.create(output_rectangle.extents().size)));

    EXPECT_EQ(windows.size(), placements.size());
    for (size_t i = 0; i < windows.size(); i++)
    {
        auto const& window = windows[i];
        const auto& expected_rect = placements[i].expected_rect;

        auto const expected = expected_rect(output_rectangle.extents().size);
        EXPECT_EQ(window.top_left(), expected.top_left);
        EXPECT_EQ(window.size(), expected.size);
    }
}

INSTANTIATE_TEST_SUITE_P(MinimalWindowManagerAttachedTestPlacement, MinimalWindowManagerAttachedTest, ::testing::Values(
    AttachedSurfacePlacementCase{
        .name="Top_Left",
        .placements={
            AttachedSurfacePlacementCase::AttachedSurfacePlacement{
                .create=create_top_attached(),
                .expected_rect=[](geom::Size const& output_size)
                {
                    // Expect that the top surface does not change at all
                    return geom::Rectangle{
                        geom::Point{0, 0},
                        geom::Size{output_size.width.as_int(), exclusive_surface_size}
                    };
                }
            },
            AttachedSurfacePlacementCase::AttachedSurfacePlacement{
                .create=create_left_attached(),
                .expected_rect=[](geom::Size const& output_size)
                {
                    // Expect that the left surface is moved down by [exclusive_surface_size] in the y-dimension
                    return geom::Rectangle{
                        geom::Point{0, exclusive_surface_size},
                        geom::Size{exclusive_surface_size, output_size.height.as_int() - exclusive_surface_size}
                    };
                }
            }
        }
    },
    AttachedSurfacePlacementCase{
        .name="Top_Right",
        .placements={
            AttachedSurfacePlacementCase::AttachedSurfacePlacement{
                .create=create_top_attached(),
                .expected_rect=[](geom::Size const& output_size)
                {
                    // Expect that the top surface does not change at all
                    return geom::Rectangle{
                        geom::Point{0, 0},
                        geom::Size{output_size.width.as_int(), exclusive_surface_size}
                    };
                }
            },
            AttachedSurfacePlacementCase::AttachedSurfacePlacement{
                .create=create_right_attached(),
                .expected_rect=[](geom::Size const& output_size)
                {
                    // Expect that the right surface is moved down by [exclusive_surface_size] in the y-dimension
                    return geom::Rectangle{
                        geom::Point{output_size.width.as_int() - exclusive_surface_size, exclusive_surface_size},
                        geom::Size{exclusive_surface_size, output_size.height.as_int() - exclusive_surface_size}
                    };
                }
            }
        }
    },
    AttachedSurfacePlacementCase{
        .name="Bottom_Left",
        .placements={
            AttachedSurfacePlacementCase::AttachedSurfacePlacement{
                .create=create_bottom_attached(),
                .expected_rect=[](geom::Size const& output_size)
                {
                    // Expect that the bottom surface does not change at all
                    return geom::Rectangle{
                        geom::Point{0, output_size.height.as_int() - exclusive_surface_size},
                        geom::Size{output_size.width, exclusive_surface_size}
                    };
                }
            },
            AttachedSurfacePlacementCase::AttachedSurfacePlacement{
                .create=create_left_attached(),
                .expected_rect=[](geom::Size const& output_size)
                {
                    // Expect that the left surface shrinks its height by [exclusive_surface_size]
                    return geom::Rectangle{
                        geom::Point{0, 0},
                        geom::Size{exclusive_surface_size, output_size.height.as_int() - exclusive_surface_size}
                    };
                }
            }
        }
    },
    AttachedSurfacePlacementCase{
        .name="Bottom_Right",
        .placements={
            AttachedSurfacePlacementCase::AttachedSurfacePlacement{
                .create=create_bottom_attached(),
                .expected_rect=[](geom::Size const& output_size)
                {
                    // Expect that the bottom surface does not change at all
                    return geom::Rectangle{
                        geom::Point{0, output_size.height.as_int() - exclusive_surface_size},
                        geom::Size{output_size.width, exclusive_surface_size}
                    };
                }
            },
            AttachedSurfacePlacementCase::AttachedSurfacePlacement{
                .create=create_right_attached(),
                .expected_rect=[](geom::Size const& output_size)
                {
                    // Expect that the right surface shrinks its height by [exclusive_surface_size]
                    return geom::Rectangle{
                        geom::Point{output_size.width.as_int() - exclusive_surface_size, 0},
                        geom::Size{exclusive_surface_size, output_size.height.as_int() - exclusive_surface_size}
                    };
                }
            }
        }
    },
    AttachedSurfacePlacementCase{
        .name="All",
        .placements={
            AttachedSurfacePlacementCase::AttachedSurfacePlacement{
                .create=create_top_attached(),
                .expected_rect=[](geom::Size const& output_size)
                {
                    // Expect that the top surface does not change at all
                    return geom::Rectangle{
                        geom::Point{0, 0},
                        geom::Size{output_size.width, exclusive_surface_size}
                    };
                }
            },
            AttachedSurfacePlacementCase::AttachedSurfacePlacement{
                .create=create_bottom_attached(),
                .expected_rect=[](geom::Size const& output_size)
                {
                    // Expect that the bottom surface does not change at all
                    return geom::Rectangle{
                        geom::Point{0, output_size.height.as_int() - exclusive_surface_size},
                        geom::Size{output_size.width, exclusive_surface_size}
                    };
                }
            },
            AttachedSurfacePlacementCase::AttachedSurfacePlacement{
                .create=create_left_attached(),
                .expected_rect=[](geom::Size const& output_size)
                {
                    // Expect that the left surface shrinks its height by 2 * [exclusive_surface_size]
                    return geom::Rectangle{
                        geom::Point{0, exclusive_surface_size},
                        geom::Size{exclusive_surface_size, output_size.height.as_int() - 2 * exclusive_surface_size}
                    };
                }
            },
            AttachedSurfacePlacementCase::AttachedSurfacePlacement{
                .create=create_right_attached(),
                .expected_rect=[](geom::Size const& output_size)
                {
                    // Expect that the right surface shrinks its height by 2 * [exclusive_surface_size]
                    return geom::Rectangle{
                        geom::Point{output_size.width.as_int() - exclusive_surface_size, exclusive_surface_size},
                        geom::Size{exclusive_surface_size, output_size.height.as_int() - 2 * exclusive_surface_size}
                    };
                }
            }
        }
    },
    AttachedSurfacePlacementCase{
        .name="Left_Right",
        .placements={
            AttachedSurfacePlacementCase::AttachedSurfacePlacement{
                .create=create_left_attached(),
                .expected_rect=[](geom::Size const& output_size)
                {
                    // Expect that the left surface does not change at all
                    return geom::Rectangle{
                        geom::Point{0, 0},
                        geom::Size{exclusive_surface_size, output_size.height.as_int()}
                    };
                }
            },
            AttachedSurfacePlacementCase::AttachedSurfacePlacement{
                .create=create_right_attached(),
                .expected_rect=[](geom::Size const& output_size)
                {
                    // Expect that the right surface does not change at all
                    return geom::Rectangle{
                        geom::Point{output_size.width.as_int() - exclusive_surface_size, 0},
                        geom::Size{exclusive_surface_size, output_size.height.as_int()}
                    };
                }
            }
        }
    },
    AttachedSurfacePlacementCase{
        .name="Top_Bottom",
        .placements={
            AttachedSurfacePlacementCase::AttachedSurfacePlacement{
                .create=create_top_attached(),
                .expected_rect=[](geom::Size const& output_size)
                {
                    // Expect that the top surface does not change at all
                    return geom::Rectangle{
                        geom::Point{0, 0},
                        geom::Size{output_size.width, exclusive_surface_size}
                    };
                }
            },
            AttachedSurfacePlacementCase::AttachedSurfacePlacement{
                .create=create_bottom_attached(),
                .expected_rect=[](geom::Size const& output_size)
                {
                    // Expect that the bottom surface does not change at all
                    return geom::Rectangle{
                        geom::Point{0, output_size.height.as_int() - exclusive_surface_size},
                        geom::Size{output_size.width, exclusive_surface_size}
                    };
                }
            }
        }
    }
), AttachedSurfacePlacementCaseToString());

struct MaximizedSurfaceExclusionZonesCase
{
    std::vector<CreateSurfaceSpecFunc> exclusive_surface_create_func;
    std::function<geom::Rectangle(geom::Rectangle const&)> expected_rectangle;
};

class MinimalWindowManagerMaximizedSurfaceExclusionZoneTest
    : public MinimalWindowManagerTest,
      public ::testing::WithParamInterface<MaximizedSurfaceExclusionZonesCase>
{
};

TEST_P(MinimalWindowManagerMaximizedSurfaceExclusionZoneTest, maximized_windows_respect_exclusive_areas)
{
    auto const app = open_application("test");
    auto const param = GetParam();
    auto const output_rectangles = get_initial_output_configs();

    std::vector<miral::Window> windows;
    auto const& output_rectangle = output_rectangles[0];
    for (const auto& create : param.exclusive_surface_create_func)
        windows.push_back(create_window(app, create(output_rectangle.extents().size)));

    EXPECT_EQ(windows.size(), param.exclusive_surface_create_func.size());
    miral::WindowSpecification spec;
    spec.state() = mir_window_state_maximized;
    spec.depth_layer() = mir_depth_layer_application;
    auto const window = create_window(app, spec);
    auto const expected = param.expected_rectangle(output_rectangle.extents());

    EXPECT_EQ(window.top_left(), expected.top_left);
    EXPECT_EQ(window.size(), expected.size);
}

INSTANTIATE_TEST_SUITE_P(MinimalWindowManagerMaximizedSurfaceExclusionZoneTest,
    MinimalWindowManagerMaximizedSurfaceExclusionZoneTest,
    ::testing::Values(
        MaximizedSurfaceExclusionZonesCase{
            .exclusive_surface_create_func={create_top_attached()},
            .expected_rectangle=[](geom::Rectangle const& ouput_rect)
            {
                return geom::Rectangle{
                    geom::Point(0, exclusive_surface_size),
                    geom::Size(
                        ouput_rect.size.width,
                        ouput_rect.size.height.as_int() - exclusive_surface_size)
                };
            }
        },
        MaximizedSurfaceExclusionZonesCase{
            .exclusive_surface_create_func={create_left_attached()},
            .expected_rectangle=[](geom::Rectangle const& ouput_rect)
            {
                return geom::Rectangle{
                    geom::Point(exclusive_surface_size, 0),
                    geom::Size(
                        ouput_rect.size.width.as_int() - exclusive_surface_size,
                        ouput_rect.size.height.as_int())
                };
            }
        },
        MaximizedSurfaceExclusionZonesCase{
            .exclusive_surface_create_func={create_right_attached()},
            .expected_rectangle=[](geom::Rectangle const& ouput_rect)
            {
                return geom::Rectangle{
                    geom::Point(0, 0),
                    geom::Size(
                        ouput_rect.size.width.as_int() - exclusive_surface_size,
                        ouput_rect.size.height.as_int())
                };
            }
        },
        MaximizedSurfaceExclusionZonesCase{
            .exclusive_surface_create_func={create_bottom_attached()},
            .expected_rectangle=[](geom::Rectangle const& ouput_rect)
            {
                return geom::Rectangle{
                    geom::Point(0, 0),
                    geom::Size(
                        ouput_rect.size.width.as_int(),
                        ouput_rect.size.height.as_int() - exclusive_surface_size)
                };
            }
        },
        MaximizedSurfaceExclusionZonesCase{
            .exclusive_surface_create_func={create_top_attached(), create_bottom_attached()},
            .expected_rectangle=[](geom::Rectangle const& ouput_rect)
            {
                return geom::Rectangle{
                    geom::Point(0, exclusive_surface_size),
                    geom::Size(
                        ouput_rect.size.width.as_int(),
                        ouput_rect.size.height.as_int() - 2 * exclusive_surface_size)
                };
            }
        },
        MaximizedSurfaceExclusionZonesCase{
            .exclusive_surface_create_func={create_left_attached(), create_right_attached()},
            .expected_rectangle=[](geom::Rectangle const& ouput_rect)
            {
                return geom::Rectangle{
                    geom::Point(exclusive_surface_size, 0),
                    geom::Size(
                        ouput_rect.size.width.as_int() - 2 * exclusive_surface_size,
                        ouput_rect.size.height.as_int())
                };
            }
        },
        MaximizedSurfaceExclusionZonesCase{
            .exclusive_surface_create_func={create_left_attached(), create_top_attached()},
            .expected_rectangle=[](geom::Rectangle const& ouput_rect)
            {
                return geom::Rectangle{
                    geom::Point(exclusive_surface_size, exclusive_surface_size),
                    geom::Size(
                        ouput_rect.size.width.as_int() - exclusive_surface_size,
                        ouput_rect.size.height.as_int() - exclusive_surface_size)
                };
            }
        },
        MaximizedSurfaceExclusionZonesCase{
            .exclusive_surface_create_func={create_right_attached(), create_bottom_attached()},
            .expected_rectangle=[](geom::Rectangle const& ouput_rect)
            {
                return geom::Rectangle{
                    geom::Point(0, 0),
                    geom::Size(
                        ouput_rect.size.width.as_int() - exclusive_surface_size,
                        ouput_rect.size.height.as_int() - exclusive_surface_size)
                };
            }
        }
    )
);

TEST_F(MinimalWindowManagerTest, when_no_surface_is_focused_then_window_is_placed_on_output_of_cursor)
{
    // Move the cursor to the second output
    auto const second_rectangle = get_initial_output_configs()[1].extents();
    geom::PointF const new_cursor_position{
        second_rectangle.top_left.x.as_int() + static_cast<int>(
            static_cast<float>(second_rectangle.size.width.as_int()) / 2.f),
        second_rectangle.top_left.y.as_int() + static_cast<int>(
            static_cast<float>(second_rectangle.size.height.as_int()) / 2.f)
    };
    tools().move_cursor_to(new_cursor_position);
    EXPECT_THAT(tools().active_output(), second_rectangle);

    auto const app = open_application("test");
    miral::WindowSpecification spec;
    spec.size() = geom::Size(100, 100);
    auto const window = create_window(app, spec);

    // Expect that the new window is centered in the X-axis and that the Y is at the expected poisition.
    EXPECT_THAT(window.top_left(), Eq(
        mir::geometry::Point(
            second_rectangle.top_left.x.as_int() + (second_rectangle.size.width.as_int() - spec.size().value().width.as_int()) / 2.f,
            167
        )
    ));
}

TEST_F(MinimalWindowManagerTest, window_can_be_maximized_on_new_output)
{
    // Add a third output
    auto const new_output_configs = output_configs_from_output_rectangles({
        mir::geometry::Rectangle{{0, 0}, {800, 600}},
        mir::geometry::Rectangle{{800, 0}, {1000, 600}},
        mir::geometry::Rectangle{{1800, 0}, {500, 500}},
    });

    update_outputs(new_output_configs);
    std::vector<miral::Output> outputs;
    for_each_output([&](miral::Output const& output)
    {
        outputs.push_back(output);
    });

    EXPECT_THAT(outputs.size(), Eq(3));

    // Move the cursor to the third output
    geom::PointF const new_cursor_position(1900, 100);
    tools().move_cursor_to(new_cursor_position);
    EXPECT_THAT(tools().active_output(), new_output_configs[2].extents());

    auto const app = open_application("test");
    miral::WindowSpecification spec;
    spec.state() = mir_window_state_maximized;
    spec.size() = geom::Size(100, 100);
    auto const window = create_window(app, spec);

    // Expect that the new window is on the third output
    EXPECT_THAT(window.top_left(), Eq(new_output_configs[2].extents().top_left));
}

TEST_F(MinimalWindowManagerTest, maximized_window_respects_output_resize)
{
    auto const app = open_application("test");
    miral::WindowSpecification spec;
    spec.state() = mir_window_state_maximized;
    spec.size() = geom::Size(100, 100);
    auto const window = create_window(app, spec);

    auto const new_output_configs = output_configs_from_output_rectangles({
        mir::geometry::Rectangle{{0, 0}, {1000, 1000}},
        mir::geometry::Rectangle{{1000, 0}, {1000, 600}}
    });
    update_outputs(new_output_configs);
    EXPECT_THAT(window.top_left(), Eq(new_output_configs[0].extents().top_left));
    EXPECT_THAT(window.size(), Eq(new_output_configs[0].extents().size));
}

TEST_F(MinimalWindowManagerTest, maximized_window_respects_output_removal)
{
    tools().move_cursor_to(geom::PointF(900, 100));

    auto const app = open_application("test");
    miral::WindowSpecification spec;
    spec.state() = mir_window_state_maximized;
    spec.size() = geom::Size(100, 100);
    auto const window = create_window(app, spec);

    auto const outputs = get_initial_output_configs();
    EXPECT_THAT(window.top_left(), Eq(outputs[1].extents().top_left));
    EXPECT_THAT(window.size(), Eq(outputs[1].extents().size));

    auto const new_output_configs = output_configs_from_output_rectangles({
        mir::geometry::Rectangle{{0, 0}, {1000, 1000}}
    });
    update_outputs(new_output_configs);
    EXPECT_THAT(window.top_left(), Eq(new_output_configs[0].extents().top_left));
    EXPECT_THAT(window.size(), Eq(new_output_configs[0].extents().size));
}

TEST_F(MinimalWindowManagerTest, maximized_window_respects_output_unused)
{
    EXPECT_NONFATAL_FAILURE({
        this->tools().move_cursor_to(geom::PointF(0, 0));
        auto const app = this->open_application("test");
        miral::WindowSpecification spec;
        spec.state() = mir_window_state_maximized;
        spec.size() = geom::Size(100, 100);
        auto const window = this->create_window(app, spec);

        auto new_output_configs = this->get_initial_output_configs();
        new_output_configs[0].used = false;
        this->update_outputs(new_output_configs);
        geom::Rectangle const window_rect(window.top_left(), window.size());
        EXPECT_THAT(window_rect, Eq(new_output_configs[1].extents()));
    }, "");
}

TEST_F(MinimalWindowManagerTest, maximized_window_respects_output_disconnected)
{
    EXPECT_NONFATAL_FAILURE({
        this->tools().move_cursor_to(geom::PointF(0, 0));
        auto const app = this->open_application("test");
        miral::WindowSpecification spec;
        spec.state() = mir_window_state_maximized;
        spec.size() = geom::Size(100, 100);
        auto const window = this->create_window(app, spec);

        auto new_output_configs = this->get_initial_output_configs();
        new_output_configs[0].connected = false;
        this->update_outputs(new_output_configs);

        geom::Rectangle const window_rect(window.top_left(), window.size());
        EXPECT_THAT(window_rect, Eq(new_output_configs[1].extents()));
    }, "");
}

TEST_F(MinimalWindowManagerTest, windows_on_removed_output_are_placed_on_next_available_output)
{
    EXPECT_NONFATAL_FAILURE({
        this->tools().move_cursor_to(geom::PointF(900, 100));

        auto const app = this->open_application("test");
        miral::WindowSpecification spec;
        spec.size() = geom::Size(100, 100);
        auto const window = this->create_window(app, spec);

        auto const new_output_configs = this->output_configs_from_output_rectangles({
            mir::geometry::Rectangle{{0, 0}, {1000, 1000}}
        });
        this->update_outputs(new_output_configs);
        geom::Rectangle const window_rect(window.top_left(), window.size());
        EXPECT_THAT(window_rect.overlaps(new_output_configs[0].extents()), Eq(true));
    }, "");
}
