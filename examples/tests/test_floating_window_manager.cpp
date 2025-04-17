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

#include <examples/example-server-lib/floating_window_manager.h>
#include <examples/example-server-lib/splash_session.h>

#include <miral/internal_client.h>
#include <mir_test_framework/window_management_test_harness.h>

#include <gmock/gmock.h>

#include <linux/input-event-codes.h>

namespace geom = mir::geometry;
using namespace testing;

class StubSplashSession : public SplashSession
{
public:
    auto session() const -> std::shared_ptr<mir::scene::Session> override
    {
        return nullptr;
    }
};

class FloatingWindowManagerTest : public mir_test_framework::WindowManagementTestHarness
{
public:
    void SetUp() override
    {
        launcher(server);
        WindowManagementTestHarness::SetUp();
    }

    auto get_builder() -> mir_test_framework::WindowManagementPolicyBuilder override
    {
        return [&](miral::WindowManagerTools const& tools)
        {
            return std::make_unique<FloatingWindowManagerPolicy>(
                tools,
                std::make_shared<StubSplashSession>(),
                launcher,
                shutdown_hook
            );
        };
    }

    auto get_output_rectangles() -> std::vector<mir::geometry::Rectangle> override
    {
        return {
            mir::geometry::Rectangle{{0, 0}, {800, 600}},
            mir::geometry::Rectangle{{800, 0}, {800, 600}}
        };
    }

private:
    miral::InternalClientLauncher launcher;
    std::function<void()> shutdown_hook = []{};
};

TEST_F(FloatingWindowManagerTest, switching_workspaces_causes_windows_to_be_hidden_and_shown)
{
    auto const app = open_application("app");
    miral::WindowSpecification spec;
    spec.size() = geom::Size(100, 100);
    spec.top_left() = geom::Point(50, 50);
    auto const window = create_window(app, spec);

    // Dispatch an f2 event
    {
        std::chrono::nanoseconds const event_timestamp = std::chrono::system_clock::now().time_since_epoch();
        MirKeyboardAction const action{mir_keyboard_action_down};
        xkb_keysym_t const keysym{0};
        int const scan_code{KEY_F2};
        MirInputEventModifiers const modifiers{mir_input_event_modifier_alt | mir_input_event_modifier_meta};
        auto const event = mir::events::make_key_event(
            mir_input_event_type_key,
            event_timestamp,
            action,
            keysym,
            scan_code,
            modifiers);
        publish_event(*event);
    }

    // Assert [window] is hidden
    auto const& info = tools().info_for(window);
    EXPECT_THAT(info.state(), Eq(mir_window_state_hidden));

    // Dispatch an f1 event
    {
        std::chrono::nanoseconds const event_timestamp = std::chrono::system_clock::now().time_since_epoch();
        MirKeyboardAction const action{mir_keyboard_action_down};
        xkb_keysym_t const keysym{0};
        int const scan_code{KEY_F1};
        MirInputEventModifiers const modifiers{mir_input_event_modifier_alt | mir_input_event_modifier_meta};
        auto const event = mir::events::make_key_event(
            mir_input_event_type_key,
            event_timestamp,
            action,
            keysym,
            scan_code,
            modifiers);
        publish_event(*event);
    }

    // Assert [window] is restored
    EXPECT_THAT(info.state(), Eq(mir_window_state_restored));
}

TEST_F(FloatingWindowManagerTest, can_switch_workspaces_and_bring_window_with_us)
{
    auto const app = open_application("app");
    miral::WindowSpecification spec;
    spec.size() = geom::Size(100, 100);
    spec.top_left() = geom::Point(50, 50);
    auto const window = create_window(app, spec);

    // Dispatch an f2 event
    std::chrono::nanoseconds const event_timestamp = std::chrono::system_clock::now().time_since_epoch();
    MirKeyboardAction const action{mir_keyboard_action_down};
    xkb_keysym_t const keysym{0};
    int const scan_code{KEY_F2};
    MirInputEventModifiers const modifiers{mir_input_event_modifier_ctrl | mir_input_event_modifier_meta};
    auto const event = mir::events::make_key_event(
        mir_input_event_type_key,
        event_timestamp,
        action,
        keysym,
        scan_code,
        modifiers);
    publish_event(*event);

    // Assert [window] is still shown
    auto const& info = tools().info_for(window);
    EXPECT_THAT(info.state(), Eq(mir_window_state_restored));
}

TEST_F(FloatingWindowManagerTest, switching_workspaces_changes_window_focus)
{
    // Create the first window on workspace f1
    auto const app1 = open_application("app1");
    miral::WindowSpecification spec;
    spec.size() = geom::Size(100, 100);
    spec.top_left() = geom::Point(50, 50);
    auto const window1 = create_window(app1, spec);
    EXPECT_THAT(focused(window1), Eq(true));

    // Dispatch an f2 event
    {
        std::chrono::nanoseconds const event_timestamp = std::chrono::system_clock::now().time_since_epoch();
        MirKeyboardAction const action{mir_keyboard_action_down};
        xkb_keysym_t const keysym{0};
        int const scan_code{KEY_F2};
        MirInputEventModifiers const modifiers{mir_input_event_modifier_alt | mir_input_event_modifier_meta};
        auto const event = mir::events::make_key_event(
            mir_input_event_type_key,
            event_timestamp,
            action,
            keysym,
            scan_code,
            modifiers);
        publish_event(*event);
    }
    EXPECT_THAT(focused({}), Eq(true));

    // Create the second window on workspace f2
    auto const app2 = open_application("app2");
    auto const window2 = create_window(app2, spec);
    EXPECT_THAT(focused(window2), Eq(true));

    // Dispatch an f1 event
    {
        std::chrono::nanoseconds const event_timestamp = std::chrono::system_clock::now().time_since_epoch();
        MirKeyboardAction const action{mir_keyboard_action_down};
        xkb_keysym_t const keysym{0};
        int const scan_code{KEY_F1};
        MirInputEventModifiers const modifiers{mir_input_event_modifier_alt | mir_input_event_modifier_meta};
        auto const event = mir::events::make_key_event(
            mir_input_event_type_key,
            event_timestamp,
            action,
            keysym,
            scan_code,
            modifiers);
        publish_event(*event);
    }

    EXPECT_THAT(focused(window1), Eq(true));
}

TEST_F(FloatingWindowManagerTest, child_windows_are_hidden_if_parent_is_on_hidden_workspace)
{
    auto const app = open_application("app");
    miral::WindowSpecification spec;
    spec.size() = geom::Size(100, 100);
    spec.top_left() = geom::Point(50, 50);
    auto const parent = create_window(app, spec);

    // Dispatch an f2 event
    std::chrono::nanoseconds const event_timestamp = std::chrono::system_clock::now().time_since_epoch();
    MirKeyboardAction const action{mir_keyboard_action_down};
    xkb_keysym_t const keysym{0};
    int const scan_code{KEY_F2};
    MirInputEventModifiers const modifiers{mir_input_event_modifier_alt | mir_input_event_modifier_meta};
    auto const event = mir::events::make_key_event(
        mir_input_event_type_key,
        event_timestamp,
        action,
        keysym,
        scan_code,
        modifiers);
    publish_event(*event);
    EXPECT_THAT(tools().info_for(parent).state(), Eq(mir_window_state_hidden));

    // Assert [child] is also hidden
    spec.parent() = parent;
    auto const child = create_window(app, spec);
    EXPECT_THAT(tools().info_for(child).state(), Eq(mir_window_state_hidden));
}

TEST_F(FloatingWindowManagerTest, cannot_modify_state_of_window_on_different_workspace)
{
    auto const app = open_application("app");
    miral::WindowSpecification spec;
    spec.size() = geom::Size(100, 100);
    spec.top_left() = geom::Point(50, 50);
    auto const window = create_window(app, spec);

    // Dispatch an f2 event
    std::chrono::nanoseconds const event_timestamp = std::chrono::system_clock::now().time_since_epoch();
    MirKeyboardAction const action{mir_keyboard_action_down};
    xkb_keysym_t const keysym{0};
    int const scan_code{KEY_F2};
    MirInputEventModifiers const modifiers{mir_input_event_modifier_alt | mir_input_event_modifier_meta};
    auto const event = mir::events::make_key_event(
        mir_input_event_type_key,
        event_timestamp,
        action,
        keysym,
        scan_code,
        modifiers);
    publish_event(*event);
    EXPECT_THAT(tools().info_for(window).state(), Eq(mir_window_state_hidden));

    miral::WindowSpecification new_spec;
    new_spec.state() = mir_window_state_restored;
    request_modify(window, new_spec);
    EXPECT_THAT(tools().info_for(window).state(), Eq(mir_window_state_hidden));
}
