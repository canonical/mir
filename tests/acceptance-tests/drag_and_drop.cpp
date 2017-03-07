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
#include <mir/test/doubles/wrap_shell_to_track_latest_surface.h>
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
using mir::test::doubles::WrapShellToTrackLatestSurface;

namespace
{
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

struct SurfaceTracker
{
    auto latest_surface() -> std::shared_ptr<mir::scene::Surface>;

    explicit SurfaceTracker(mir::Server& server);

private:
    std::weak_ptr<WrapShellToTrackLatestSurface> shell_;
};

SurfaceTracker::SurfaceTracker(mir::Server& server)
{
    using mir::shell::Shell;
    server.wrap_shell([&](std::shared_ptr<Shell> const& wrapped) -> std::shared_ptr<Shell>
        {
            auto const msc = std::make_shared<WrapShellToTrackLatestSurface>(wrapped);
            shell_ = msc;
            return msc;
        });
}

auto SurfaceTracker::latest_surface() -> std::shared_ptr<mir::scene::Surface>
{
    auto const shell = shell_.lock();
    if (!shell) BOOST_THROW_EXCEPTION(std::logic_error("no shell"));

    auto const result = shell->latest_surface.lock();
    if (!result) BOOST_THROW_EXCEPTION(std::logic_error("no surface"));

    return result;
}

Rectangle const screen_geometry{{0,0}, {800,600}};
auto const receive_event_timeout = 3s;  // TODO change to 30s before landing

struct DragAndDrop : mir_test_framework::ConnectedClientWithAWindow,
                     MouseMoverAndFaker, SurfaceTracker
{
    DragAndDrop() : SurfaceTracker{server} {}

    MirDragAndDropV1 const* dnd = nullptr;
    Signal initiated;

    std::shared_ptr<mir::scene::Surface> scene_surface;

    void SetUp() override
    {
        initial_display_layout({screen_geometry});
        mir_test_framework::ConnectedClientWithAWindow::SetUp();

        dnd = mir_drag_and_drop_v1(connection);

        paint_window();
        mir_window_set_event_handler(window, &window_event_handler, this);
        scene_surface = latest_surface();

        center_mouse();
    }

    void TearDown() override
    {
        scene_surface.reset();
        mir_test_framework::ConnectedClientWithAWindow::TearDown();
    }

    void signal_when_initiated(MirWindow* window, MirEvent const* event);

    void set_window_event_handler(std::function<void(MirWindow* window, MirEvent const* event)> const& handler);

    auto user_initiates_drag() -> MirCookie const*;

private:
    void center_mouse() { move_mouse(0.5 * as_displacement(screen_geometry.size)); }
    void paint_window() const { mir_buffer_stream_swap_buffers_sync(mir_window_get_buffer_stream(window)); }

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

void DragAndDrop::signal_when_initiated(MirWindow* window, MirEvent const* event)
{
    EXPECT_THAT(window, Eq(this->window));

    if (mir_event_get_type(event) != mir_event_type_window)
        return;

    if (!dnd) return;

    if (auto handle = dnd->start_drag(mir_event_get_window_event(event)))
    {
        initiated.raise();
        mir_blob_release(handle);
    }
}

auto DragAndDrop::user_initiates_drag() -> MirCookie const*
{
    MirCookie const* cookie = nullptr;
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

            cookie = mir_input_event_get_cookie(input_event);
            have_cookie.raise();
        });

    start_dragging_mouse();

    if (!have_cookie.wait_for(receive_event_timeout)) BOOST_THROW_EXCEPTION(std::logic_error("no cookie"));

    return cookie;
}
}

TEST_F(DragAndDrop, DISABLED_can_initiate)
{
    MirCookie const* cookie = user_initiates_drag();

    set_window_event_handler([this](MirWindow* window, MirEvent const* event)
        { signal_when_initiated(window, event); });

    if (!dnd) BOOST_THROW_EXCEPTION(std::logic_error("no dnd extension"));

    dnd->request_drag_and_drop(window, cookie);

    EXPECT_TRUE(initiated.wait_for(receive_event_timeout));
}
