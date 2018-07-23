/*
 * Copyright © 2017 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include <mir_toolkit/mir_window.h>
#include <mir_toolkit/mir_blob.h>

#include <mir/geometry/displacement.h>
#include <mir/input/input_device_info.h>
#include <mir/input/device_capability.h>

#include <mir_test_framework/canonical_window_manager_policy.h>
#include <mir_test_framework/connected_client_with_a_window.h>
#include <mir_test_framework/fake_input_device.h>
#include <mir_test_framework/stub_server_platform_factory.h>
#include <mir/test/event_factory.h>
#include <mir/test/fake_shared.h>
#include <mir/test/signal.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <linux/input.h>

#include <atomic>

using namespace std::chrono_literals;
using namespace mir::geometry;
using namespace testing;
using mir::test::fake_shared;
using mir::test::Signal;

namespace
{
class Cookie
{
public:
    Cookie() = default;

    explicit Cookie(MirCookie const* cookie) : self{cookie, deleter} {}

    operator MirCookie const*() const { return self.get(); }

    auto get() const -> MirCookie const* { return self.get(); }

    void reset() { self.reset(); }

    void reset(MirCookie const* cookie) { self.reset(cookie, deleter); }

private:
    static void deleter(MirCookie const* cookie) { mir_cookie_release(cookie); }

    std::shared_ptr<MirCookie const> self;
};

void mir_cookie_release(Cookie const&) = delete;

struct MockWindowManagementPolicy : mir_test_framework::CanonicalWindowManagerPolicy
{
    MockWindowManagementPolicy(
        miral::WindowManagerTools const& tools,
        MockWindowManagementPolicy*& self) :
        mir_test_framework::CanonicalWindowManagerPolicy(tools)
        {
            self = this;
        }

    MOCK_METHOD2(handle_request_move, void(miral::WindowInfo&, MirInputEvent const*));
    MOCK_METHOD1(handle_request_drag_and_drop, void(miral::WindowInfo&));
    MOCK_METHOD3(handle_request_resize, void(miral::WindowInfo&, MirInputEvent const*, MirResizeEdge));
};

struct MouseMoverAndFaker
{
    void start_dragging_mouse()
    {
        using namespace mir::input::synthesis;
        fake_mouse->emit_event(a_button_down_event().of_button(BTN_LEFT));
    }

    void move_mouse(Displacement const& displacement)
    {
        using mir::input::synthesis::a_pointer_event;
        fake_mouse->emit_event(a_pointer_event().with_movement(displacement.dx.as_int(), displacement.dy.as_int()));
    }

    void release_mouse()
    {
        using namespace mir::input::synthesis;
        fake_mouse->emit_event(a_button_up_event().of_button(BTN_LEFT));
    }

private:
    std::unique_ptr<mir_test_framework::FakeInputDevice> fake_mouse{
        mir_test_framework::add_fake_input_device(
            mir::input::InputDeviceInfo{"mouse", "mouse-uid", mir::input::DeviceCapability::pointer})
    };
};

Rectangle const screen_geometry{{0,   0}, {800, 600}};
auto const receive_event_timeout = 90s;

struct ClientMediatedUserGestures : mir_test_framework::ConnectedClientWithAWindow,
                                    MouseMoverAndFaker
{
    void SetUp() override
    {
        initial_display_layout({screen_geometry});
        override_window_management_policy<MockWindowManagementPolicy>(mock_wm_policy);
        mir_test_framework::ConnectedClientWithAWindow::SetUp();
        ASSERT_THAT(mock_wm_policy, NotNull());

        mir_window_set_event_handler(window, &window_event_handler, this);

        paint_window();

        center_mouse();
    }

    void TearDown() override
    {
        reset_window_event_handler();
        mir_test_framework::ConnectedClientWithAWindow::TearDown();
    }

    auto user_initiates_gesture() -> Cookie;

    MockWindowManagementPolicy* mock_wm_policy = 0;

private:
    void center_mouse();
    void paint_window();
    void set_window_event_handler(std::function<void(MirEvent const* event)> const& handler);
    void reset_window_event_handler();
    void invoke_window_event_handler(MirEvent const* event)
    {
        std::lock_guard<decltype(window_event_handler_mutex)> lock{window_event_handler_mutex};
        window_event_handler_(event);
    }

    std::mutex window_event_handler_mutex;
    std::function<void(MirEvent const* event)> window_event_handler_ = [](MirEvent const*) {};

    static void window_event_handler(MirWindow* window, MirEvent const* event, void* context);
};

void ClientMediatedUserGestures::set_window_event_handler(std::function<void(MirEvent const* event)> const& handler)
{
    std::lock_guard<decltype(window_event_handler_mutex)> lock{window_event_handler_mutex};
    window_event_handler_ = handler;
}

void ClientMediatedUserGestures::reset_window_event_handler()
{
    std::lock_guard<decltype(window_event_handler_mutex)> lock{window_event_handler_mutex};
    window_event_handler_ = [](MirEvent const*) {};
}

void ClientMediatedUserGestures::window_event_handler(MirWindow* /*window*/, MirEvent const* event, void* context)
{
    static_cast<ClientMediatedUserGestures*>(context)->invoke_window_event_handler(event);
}

void ClientMediatedUserGestures::paint_window()
{
    Signal have_focus;

    set_window_event_handler([&](MirEvent const* event)
        {
            if (mir_event_get_type(event) != mir_event_type_window)
                return;

            auto const window_event = mir_event_get_window_event(event);
            if (mir_window_event_get_attribute(window_event) != mir_window_attrib_focus)
                return;

            if (mir_window_event_get_attribute_value(window_event))
                have_focus.raise();
        });

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    mir_buffer_stream_swap_buffers_sync(mir_window_get_buffer_stream(window));
#pragma GCC diagnostic pop

    EXPECT_THAT(have_focus.wait_for(receive_event_timeout), Eq(true));

    reset_window_event_handler();
}

void ClientMediatedUserGestures::center_mouse()
{
    Signal have_mouseover;

    set_window_event_handler([&](MirEvent const* event)
        {
            if (mir_event_get_type(event) != mir_event_type_input)
                return;

            auto const input_event = mir_event_get_input_event(event);

            if (mir_input_event_get_type(input_event) != mir_input_event_type_pointer)
                return;

            auto const pointer_event = mir_input_event_get_pointer_event(input_event);

            if (mir_pointer_event_action(pointer_event) != mir_pointer_action_enter)
                return;

            have_mouseover.raise();
        });

    move_mouse(0.5 * as_displacement(screen_geometry.size));

// We miss the "mouseover" occasionally (with valgrind and heavy stress about 1/20).
// But it isn't essential for the test and we've probably waited long enough
// for the mouse-down needed by the test to reach the window.
//    EXPECT_THAT(have_mouseover.wait_for(receive_event_timeout), Eq(true));
    have_mouseover.wait_for(receive_event_timeout);

    reset_window_event_handler();
}

auto ClientMediatedUserGestures::user_initiates_gesture() -> Cookie
{
    Cookie cookie;
    Signal have_cookie;

    set_window_event_handler([&](MirEvent const* event)
        {
            if (mir_event_get_type(event) != mir_event_type_input)
                return;

            auto const input_event = mir_event_get_input_event(event);

            if (mir_input_event_get_type(input_event) != mir_input_event_type_pointer)
                return;

            auto const pointer_event = mir_input_event_get_pointer_event(input_event);

            if (mir_pointer_event_action(pointer_event) != mir_pointer_action_button_down)
                return;

            cookie = Cookie{mir_input_event_get_cookie(input_event)};
            have_cookie.raise();
        });

    start_dragging_mouse();

    EXPECT_THAT(have_cookie.wait_for(receive_event_timeout), Eq(true));

    reset_window_event_handler();
    return cookie;
}
}

// TODO extend this test when server side implemented
TEST_F(ClientMediatedUserGestures, when_client_initiates_resize_nothing_bad_happens)
{
    auto const cookie = user_initiates_gesture();

    mir_window_request_user_resize(window, mir_resize_edge_east, cookie);
}

TEST_F(ClientMediatedUserGestures, when_user_initiates_gesture_client_receives_cookie)
{
    auto const cookie = user_initiates_gesture();

    EXPECT_THAT(cookie.get(), NotNull());
}

TEST_F(ClientMediatedUserGestures, when_client_initiates_move_window_manager_handles_request)
{
    auto const cookie = user_initiates_gesture();
    Signal have_request;
    EXPECT_CALL(*mock_wm_policy, handle_request_move(_, _)).WillOnce(InvokeWithoutArgs([&]{ have_request.raise(); }));

    mir_window_request_user_move(window, cookie);

    EXPECT_THAT(have_request.wait_for(receive_event_timeout), Eq(true));
}
