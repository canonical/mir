/*
 * Copyright © 2017 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
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

#include <mir_toolkit/extensions/drag_and_drop.h>
#include <mir_toolkit/mir_blob.h>

#include <mir/geometry/displacement.h>
#include <mir/input/input_device_info.h>
#include <mir/input/device_capability.h>
#include <mir/shell/shell.h>

#include <mir_test_framework/connected_client_with_a_window.h>
#include <mir_test_framework/fake_input_device.h>
#include <mir_test_framework/stub_server_platform_factory.h>
#include <mir/test/event_factory.h>
#include <mir/test/signal.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <linux/input-event-codes.h>

#include <boost/throw_exception.hpp>

using namespace std::chrono_literals;
using namespace mir::geometry;
using namespace testing;
using mir::test::Signal;

namespace
{
class Cookie
{
public:
    Cookie() = default;
    explicit Cookie(MirCookie const* cookie) : self{cookie, deleter} {}

    operator MirCookie const*() const { return self.get(); }

    void reset() { self.reset(); }
    void reset(MirCookie const* cookie) { self.reset(cookie, deleter); }

    friend void mir_cookie_release(Cookie const&) = delete;

private:
    static void deleter(MirCookie const* cookie) { mir_cookie_release(cookie); }
    std::shared_ptr<MirCookie const> self;
};

class Blob
{
public:
    Blob() = default;
    explicit Blob(MirBlob* blob) : self{blob, deleter} {}

    operator MirBlob*() const { return self.get(); }

    void reset() { self.reset(); }
    void reset(MirBlob* blob) { self.reset(blob, deleter); }

    friend void mir_blob_release(Blob const&) = delete;

private:
    static void deleter(MirBlob* blob) { mir_blob_release(blob); }
    std::shared_ptr<MirBlob> self;
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

private:
    std::unique_ptr<mir_test_framework::FakeInputDevice> fake_mouse{
        mir_test_framework::add_fake_input_device(
            mir::input::InputDeviceInfo{"mouse", "mouse-uid", mir::input::DeviceCapability::pointer})};
};

Rectangle const screen_geometry{{0,0}, {800,600}};
auto const receive_event_timeout = 3s;  // TODO change to 30s before showing to CI

struct DragAndDrop : mir_test_framework::ConnectedClientWithAWindow,
                     MouseMoverAndFaker
{
    MirDragAndDropV1 const* dnd = nullptr;

    void SetUp() override
    {
        initial_display_layout({screen_geometry});
        mir_test_framework::ConnectedClientWithAWindow::SetUp();

        dnd = mir_drag_and_drop_v1(connection);

        paint_window();
        mir_window_set_event_handler(window, &window_event_handler, this);

        center_mouse();
    }

    void TearDown() override
    {
        set_window_event_handler([&](MirWindow*, MirEvent const*) {});
        mir_test_framework::ConnectedClientWithAWindow::TearDown();
    }

    auto user_initiates_drag() -> Cookie;
    auto client_requests_drag(Cookie const& cookie) -> Blob;
    auto handle_from_mouse_move() -> Blob;

private:
    void center_mouse() { move_mouse(0.5 * as_displacement(screen_geometry.size)); }
    void paint_window() const { mir_buffer_stream_swap_buffers_sync(mir_window_get_buffer_stream(window)); }
    void set_window_event_handler(std::function<void(MirWindow* window, MirEvent const* event)> const& handler);

    void invoke_window_event_handler(MirWindow* window, MirEvent const* event)
    {
        std::lock_guard<decltype(window_event_handler_mutex)> lock{window_event_handler_mutex};
        window_event_handler_(window, event);
    }

    std::mutex window_event_handler_mutex;
    std::function<void(MirWindow* window, MirEvent const* event)> window_event_handler_ =
        [](MirWindow*, MirEvent const*) {};

    static void window_event_handler(MirWindow* window, MirEvent const* event, void* context);
};

void DragAndDrop::set_window_event_handler(std::function<void(MirWindow* window, MirEvent const* event)> const& handler)
{
    std::lock_guard<decltype(window_event_handler_mutex)> lock{window_event_handler_mutex};
    window_event_handler_ = handler;
}

void DragAndDrop::window_event_handler(MirWindow* window, MirEvent const* event, void* context)
{
    static_cast<DragAndDrop*>(context)->invoke_window_event_handler(window, event);
}

auto DragAndDrop::user_initiates_drag() -> Cookie
{
    Cookie cookie;
    Signal have_cookie;

    set_window_event_handler([&](MirWindow*, MirEvent const* event)
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

    return cookie;
}

auto DragAndDrop::client_requests_drag(Cookie const& cookie) -> Blob
{
    Blob blob;
    Signal initiated;

    set_window_event_handler([&](MirWindow*, MirEvent const* event)
        {
            if (mir_event_get_type(event) != mir_event_type_window)
                return;

            if (!dnd) return;

            blob.reset(dnd->start_drag_and_drop(mir_event_get_window_event(event)));

            if (blob)
                initiated.raise();
        });

    EXPECT_THAT(dnd, Ne(nullptr)) << "No Drag and Drop extension";

    if (dnd)
        dnd->request_drag_and_drop(window, cookie);

    EXPECT_TRUE(initiated.wait_for(receive_event_timeout));

    return blob;
}

auto DragAndDrop::handle_from_mouse_move() -> Blob
{
    Blob blob;
    Signal have_blob;

    set_window_event_handler([&](MirWindow*, MirEvent const* event)
        {
            if (mir_event_get_type(event) != mir_event_type_input)
                return;

            auto const input_event = mir_event_get_input_event(event);

            if (mir_input_event_get_type(input_event) != mir_input_event_type_pointer)
                return;

            auto const pointer_event = mir_input_event_get_pointer_event(input_event);

            EXPECT_THAT(dnd, Ne(nullptr)) << "No Drag and Drop extension";

            if (dnd)
                blob.reset(dnd->pointer_drag_and_drop(pointer_event));

            if (blob)
                have_blob.raise();
        });

    move_mouse({1,1});

    EXPECT_TRUE(have_blob.wait_for(receive_event_timeout));
    return blob;
}
}

TEST_F(DragAndDrop, when_user_initiates_drag_client_receives_cookie)
{
    auto const cookie = user_initiates_drag();

    EXPECT_THAT(cookie, NotNull());
}

TEST_F(DragAndDrop, when_client_requests_drags_it_receives_handle)
{
    auto const cookie = user_initiates_drag();

    auto const handle = client_requests_drag(cookie);

    EXPECT_THAT(handle, NotNull());
}

TEST_F(DragAndDrop, DISABLED_during_drag_when_user_moves_mouse_client_receives_handle)
{
    auto const cookie = user_initiates_drag();

    auto const handle_from_request = client_requests_drag(cookie);

    auto const handle = handle_from_mouse_move();

    EXPECT_THAT(handle, NotNull());
    EXPECT_THAT(handle, Eq(handle_from_request));
}
