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
#include <mir/test/signal.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <boost/throw_exception.hpp>
#include <mir/test/event_factory.h>

using namespace std::chrono_literals;
using namespace testing;
using mir::shell::Shell;
using mir::test::Signal;
using mir::test::doubles::WrapShellToTrackLatestSurface;
using mir::geometry::Displacement;

namespace
{
struct MouseMoverAndFaker
{
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

struct DragAndDrop : mir_test_framework::ConnectedClientWithAWindow,
                     MouseMoverAndFaker, SurfaceTracker
{
    DragAndDrop() : SurfaceTracker{server} {}

    MirDragAndDropV1 const* dnd = nullptr;
    Signal initiated;

    MirCookie* const cookie = nullptr; // TODO get a valid cookie
    std::shared_ptr<mir::scene::Surface> scene_surface;

    void SetUp() override
    {
        mir_test_framework::ConnectedClientWithAWindow::SetUp();

        dnd = mir_drag_and_drop_v1(connection);
        ASSERT_THAT(dnd, Ne(nullptr));

        mir_window_set_event_handler(window, &signal_when_initiated, this);
        scene_surface = latest_surface();
    }

private:
    void signal_when_initiated(MirWindow* window, MirEvent const* event);
    static void signal_when_initiated(MirWindow* window, MirEvent const* event, void* context);
};

void DragAndDrop::signal_when_initiated(MirWindow* window, MirEvent const* event, void* context)
{
    static_cast<DragAndDrop*>(context)->signal_when_initiated(window, event);
}

void DragAndDrop::signal_when_initiated(MirWindow* window, MirEvent const* event)
{
    EXPECT_THAT(window, Eq(this->window));

    if (mir_event_get_type(event) != mir_event_type_window)
        return;

    if (auto handle = dnd->start_drag(mir_event_get_window_event(event)))
    {
        initiated.raise();
        mir_blob_release(handle);
    }
}
}

TEST_F(DragAndDrop, DISABLED_can_initiate)
{
    dnd->request_drag_and_drop(window, cookie);

    EXPECT_TRUE(initiated.wait_for(30s));
}
