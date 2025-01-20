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

#include "mir/geometry/forward.h"
#define MIR_LOG_COMPONENT "test_minimal_window_manager"
#include <miral/minimal_window_manager.h>
#include <mir/scene/session.h>
#include <mir/wayland/weak.h>
#include <mir/scene/surface.h>
#include <mir/executor.h>
#include <mir/events/event_builders.h>
#include <mir/events/pointer_event.h>
#include <mir/compositor/buffer_stream.h>
#include <mir/graphics/buffer_basic.h>
#include <mir/log.h>

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
    msh::SurfaceSpecification spec;
    spec.width = geom::Width {100};
    spec.height = geom::Height{100};
    spec.depth_layer = mir_depth_layer_application;
    auto window = create_window(app, spec);
    EXPECT_EQ(focused_surface(), window.operator std::shared_ptr<ms::Surface>());
}

TEST_F(MinimalWindowManagerTest, alt_f4_closes_active_window)
{
    auto const app = open_application("test");
    msh::SurfaceSpecification spec;
    spec.width = geom::Width {100};
    spec.height = geom::Height{100};
    spec.depth_layer = mir_depth_layer_application;
    auto window = create_window(app, spec);
    auto surface = window.operator std::shared_ptr<ms::Surface>();
    EXPECT_EQ(focused_surface(), surface);

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

    EXPECT_EQ(focused_surface(), nullptr);
}

TEST_F(MinimalWindowManagerTest, can_navigate_between_apps_using_alt_tab)
{
    // Create two apps, each with a single window
    auto const app1 = open_application("test");
    msh::SurfaceSpecification spec1;
    spec1.width = geom::Width {100};
    spec1.height = geom::Height{100};
    spec1.depth_layer = mir_depth_layer_application;
    auto window1 = create_window(app1, spec1);
    auto const app2 = open_application("test-1");
    msh::SurfaceSpecification spec2;
    spec2.width = geom::Width {100};
    spec2.height = geom::Height{100};
    spec2.depth_layer = mir_depth_layer_application;
    auto window2 = create_window(app2, spec2);

    EXPECT_EQ(focused_surface(), window2.operator std::shared_ptr<ms::Surface>());

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
    auto surface1 = window1.operator std::shared_ptr<ms::Surface>();
    EXPECT_EQ(focused_surface(), surface1);

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
    EXPECT_EQ(focused_surface(), surface1);
}

TEST_F(MinimalWindowManagerTest, can_navigate_within_an_app_using_alt_grave)
{
    // Create two windows on the same app
    auto const app1 = open_application("test");
    msh::SurfaceSpecification spec1;
    spec1.width = geom::Width {100};
    spec1.height = geom::Height{100};
    spec1.depth_layer = mir_depth_layer_application;
    auto window1 = create_window(app1, spec1);

    msh::SurfaceSpecification spec2;
    spec2.width = geom::Width {100};
    spec2.height = geom::Height{100};
    spec2.depth_layer = mir_depth_layer_application;
    auto window2 = create_window(app1, spec2);

    EXPECT_EQ(focused_surface(), window2.operator std::shared_ptr<ms::Surface>());

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
    auto surface1 = window1.operator std::shared_ptr<ms::Surface>();
    EXPECT_EQ(focused_surface(), surface1);

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
    EXPECT_EQ(focused_surface(), surface1);
}

TEST_F(MinimalWindowManagerTest, can_select_window_with_pointer)
{
    // Create two apps, each with a single window
    auto const app1 = open_application("test");
    msh::SurfaceSpecification spec1;
    spec1.width = geom::Width {100};
    spec1.height = geom::Height{100};
    spec1.depth_layer = mir_depth_layer_application;
    auto window1 = create_window(app1, spec1);
    auto const app2 = open_application("test-1");
    msh::SurfaceSpecification spec2;
    spec2.width = geom::Width {50};
    spec2.height = geom::Height{50};
    spec2.depth_layer = mir_depth_layer_application;
    auto window2 = create_window(app2, spec2);

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
    msh::SurfaceSpecification spec1;
    spec1.width = geom::Width {100};
    spec1.height = geom::Height{100};
    spec1.depth_layer = mir_depth_layer_application;
    auto window1 = create_window(app1, spec1);
    msh::SurfaceSpecification spec2;
    spec2.width = geom::Width {50};
    spec2.height = geom::Height{50};
    spec2.depth_layer = mir_depth_layer_application;
    auto window2 = create_window(app1, spec2);

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
    msh::SurfaceSpecification spec;
    spec.width = geom::Width {100};
    spec.height = geom::Height{100};
    spec.depth_layer = mir_depth_layer_application;
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
    mir::shell::SurfaceSpecification spec;
    spec.state = mir_window_state_maximized;
    spec.depth_layer = mir_depth_layer_application;
    spec.width = geom::Width{100};
    spec.height = geom::Height{100};
    spec.top_left = geom::Point{0, 0};
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
    msh::SurfaceSpecification spec;
    spec.width = geom::Width {100};
    spec.height = geom::Height{100};
    spec.depth_layer = mir_depth_layer_application;

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
    msh::SurfaceSpecification spec;
    spec.width = geom::Width {100};
    spec.height = geom::Height{100};
    spec.depth_layer = mir_depth_layer_application;
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
    msh::SurfaceSpecification spec;
    spec.width = geom::Width {100};
    spec.height = geom::Height{100};
    spec.depth_layer = mir_depth_layer_application;
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

TEST_F(MinimalWindowManagerTest,  new_windows_are_inserted_above_older_windows)
{
    auto const app1 = open_application("app1");
    auto const app2 = open_application("app2");
    auto const app3 = open_application("app3");
    msh::SurfaceSpecification spec;
    spec.width = geom::Width {100};
    spec.height = geom::Height{100};
    spec.depth_layer = mir_depth_layer_application;
    auto window1 = create_window(app1, spec);
    auto window2 = create_window(app2, spec);
    auto window3 = create_window(app3, spec);

    EXPECT_TRUE(is_above(window3, window2));
    EXPECT_TRUE(is_above(window2, window1));
}

TEST_F(MinimalWindowManagerTest,  when_a_window_is_focused_then_it_appears_above_other_windows)
{
    auto const app1 = open_application("app1");
    auto const app2 = open_application("app2");
    auto const app3 = open_application("app3");
    msh::SurfaceSpecification spec;
    spec.width = geom::Width {100};
    spec.height = geom::Height{100};
    spec.depth_layer = mir_depth_layer_application;
    auto window1 = create_window(app1, spec);
    auto window2 = create_window(app2, spec);
    auto window3 = create_window(app3, spec);

    request_focus(window1);

    auto surface1 = window1.operator std::shared_ptr<ms::Surface>();
    EXPECT_EQ(focused_surface(), surface1);

    EXPECT_TRUE(is_above(window1, window3));
    EXPECT_TRUE(is_above(window3, window2));
}

struct SurfacePlacementCase
{
    struct SurfacePlacement
    {
        /// The gravity that the window will be placed with
        MirPlacementGravity gravity;

        geom::Point position;

        /// If set to std::nullopt, we expect to take up the full width of the output
        std::optional<geom::Width> width;

        /// If set to std::nullopt, we expect to take up the full height of the output
        std::optional<geom::Height> height;

        /// Given the rectangle of the output, returns the expected rectangle
        std::function<geom::Rectangle(geom::Size const&)> expected;
    };


    std::vector<SurfacePlacement> placements;
};

class MinimalWindowManagerAttachedTest
    : public MinimalWindowManagerTest,
      public ::testing::WithParamInterface<SurfacePlacementCase>
{
};

TEST_P(MinimalWindowManagerAttachedTest, attached_window_positioning)
{
    auto const app = open_application("test");
    auto const placements = GetParam();
    auto const output_rectangles = get_output_rectangles();

    std::vector<miral::Window> windows;
    auto const output_width = output_rectangles[0].size.width;
    auto const output_height = output_rectangles[0].size.height;
    for (auto const& item : placements.placements)
    {
        msh::SurfaceSpecification spec;
        spec.width = item.width ? item.width.value() : output_width;
        spec.height = item.height ? item.height.value() : output_height;
        spec.top_left = item.position;
        spec.exclusive_rect = mir::optional_value(geom::Rectangle{
            item.position,
            geom::Size{
                item.width ? item.width.value() : output_width,
                item.height ? item.height.value() : output_height}});
        spec.state = mir_window_state_attached;
        spec.attached_edges = item.gravity;
        spec.depth_layer = mir_depth_layer_application;
        auto window = create_window(app, spec);
        windows.push_back(window);
    }

    EXPECT_EQ(windows.size(), placements.placements.size());
    for (size_t i = 0; i < windows.size(); i++)
    {
        auto const& window = windows[i];
        auto const& data = placements.placements[i];

        auto const expected = data.expected(output_rectangles[0].size);
        EXPECT_EQ(window.top_left(), expected.top_left);
        EXPECT_EQ(window.size(), expected.size);
    }
}

INSTANTIATE_TEST_SUITE_P(MinimalWindowManagerAttachedTestPlacement, MinimalWindowManagerAttachedTest, ::testing::Values(
    SurfacePlacementCase{
        .placements={
            // Top
            SurfacePlacementCase::SurfacePlacement{
                .gravity=MirPlacementGravity(mir_placement_gravity_north | mir_placement_gravity_east | mir_placement_gravity_west),
                .position=geom::Point{0, 0},
                .width=std::nullopt,
                .height=geom::Height{50},
                .expected=[](geom::Size const& output_size)
                {
                    return geom::Rectangle{
                        geom::Point{0, 0},
                        geom::Size{output_size.width, geom::Height{50}}
                    };
                }
            },
            // Left
            SurfacePlacementCase::SurfacePlacement{
                .gravity=MirPlacementGravity(mir_placement_gravity_north | mir_placement_gravity_south | mir_placement_gravity_west),
                .position=geom::Point{0, 0},
                .width=geom::Width{50},
                .height=std::nullopt,
                .expected=[](geom::Size const& output_size)
                {
                    return geom::Rectangle{
                        geom::Point{0, 50},
                        geom::Size{geom::Width{50}, geom::Height{output_size.height.as_int() - 50}}
                    };
                }
            }
        }
    }
));
