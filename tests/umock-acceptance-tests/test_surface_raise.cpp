/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Brandon Schaefer <brandontschaefer@canonical.com>
 */


#include "mir_test_framework/udev_environment.h"
#include "mir_test_framework/connected_client_headless_server.h"
#include "mir_test_framework/any_surface.h"
#include "mir/test/wait_condition.h"
#include "mir/test/spin_wait.h"

#include "mir_toolkit/mir_client_library.h"

#include "boost/throw_exception.hpp"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mtf = mir_test_framework;
namespace mt = mir::test;

namespace
{
std::chrono::seconds const max_wait{4};
void cookie_capturing_callback(MirSurface* surface, MirEvent const* ev, void* ctx);
}

struct RaiseSurfaces : mtf::ConnectedClientHeadlessServer
{
    RaiseSurfaces()
    {
        // Needed because the headless server sets stub_input.so
        add_to_environment("MIR_SERVER_PLATFORM_INPUT_LIB", nullptr);

        mock_devices.add_standard_device("laptop-keyboard");
    }

    void SetUp() override
    {
        ConnectedClientHeadlessServer::SetUp();

        surface1 = mtf::make_any_surface(connection);
        mir_surface_set_event_handler(surface1, &cookie_capturing_callback, this);
        mir_buffer_stream_swap_buffers_sync(
            mir_surface_get_buffer_stream(surface1));

        surface2 = mtf::make_any_surface(connection);
        mir_surface_set_event_handler(surface2, &cookie_capturing_callback, this);
        mir_buffer_stream_swap_buffers_sync(
            mir_surface_get_buffer_stream(surface2));
    }

    MirSurface* surface1;
    MirSurface* surface2;

    mtf::UdevEnvironment mock_devices;
    std::vector<MirCookie> out_cookies;
    MirSurface* out_surface{nullptr};
    mir::test::WaitCondition udev_read_recording;
};

namespace
{
void cookie_capturing_callback(MirSurface* surface, MirEvent const* ev, void* ctx)
{
    auto client = reinterpret_cast<RaiseSurfaces*>(ctx);
    auto etype = mir_event_get_type(ev);
    if (etype == mir_event_type_input)
    {
        auto iev = mir_event_get_input_event(ev);
        auto itype = mir_input_event_get_type(iev);
        if (itype == mir_input_event_type_key)
        {
            auto kev = mir_input_event_get_keyboard_event(iev);
            client->out_surface = surface;
            client->out_cookies.push_back(mir_keyboard_event_get_cookie(kev));

            // Want to make sure we read to up 2 events before we wake up
            if (client->out_cookies.size() >= 2)
                client->udev_read_recording.wake_up_everyone();
        }
    }
}
}

TEST_F(RaiseSurfaces, raise_surface_with_cookie)
{
    mock_devices.load_device_evemu("laptop-keyboard-hello");

    udev_read_recording.wait_for_at_most_seconds(max_wait);
    if (!udev_read_recording.woken())
        BOOST_THROW_EXCEPTION(std::runtime_error("Timeout waiting for udev to read the recording 'laptop-keyboard-hello'"));

    EXPECT_EQ(mir_surface_get_focus(surface2), mir_surface_focused);
    EXPECT_EQ(out_surface, surface2);

    mir_surface_raise_with_cookie(surface1, out_cookies.back());
    bool surface_becomes_focused = mt::spin_wait_for_condition_or_timeout(
        [this]
        {
            return mir_surface_get_focus(surface1) == mir_surface_focused;
        },
        std::chrono::seconds{max_wait});

    EXPECT_TRUE(surface_becomes_focused);
}

TEST_F(RaiseSurfaces, raise_surface_prevents_focus_steal)
{
    mock_devices.load_device_evemu("laptop-keyboard-hello");

    udev_read_recording.wait_for_at_most_seconds(max_wait);
    if (!udev_read_recording.woken())
        BOOST_THROW_EXCEPTION(std::runtime_error("Timeout waiting for udev to read the recording 'laptop-keyboard-hello'"));

    EXPECT_TRUE(out_cookies.front().timestamp < out_cookies.back().timestamp);

    EXPECT_EQ(mir_surface_get_focus(surface2), mir_surface_focused);
    EXPECT_EQ(out_surface, surface2);

    mir_surface_raise_with_cookie(surface1, out_cookies.front());

    // Need to wait for this call to actually go through
    std::this_thread::sleep_for(std::chrono::milliseconds{1000});

    EXPECT_EQ(mir_surface_get_focus(surface2), mir_surface_focused);
}
