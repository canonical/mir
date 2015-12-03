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

#include "mir/input/input_device_info.h"

#include "mir_test_framework/fake_input_device.h"
#include "mir_test_framework/connected_client_headless_server.h"
#include "mir_test_framework/stub_server_platform_factory.h"
#include "mir_test_framework/any_surface.h"
#include "mir/test/wait_condition.h"
#include "mir/test/spin_wait.h"
#include "mir/cookie_factory.h"

#include "mir_toolkit/mir_client_library.h"

#include <linux/input.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mtf = mir_test_framework;
namespace mt = mir::test;
namespace mi = mir::input;
namespace mis = mir::input::synthesis;

namespace
{
std::chrono::seconds const max_wait{4};
void cookie_capturing_callback(MirSurface* surface, MirEvent const* ev, void* ctx);
void lifecycle_changed(MirConnection* /* connection */, MirLifecycleState state, void* ctx);
}

struct RaiseSurfaces : mtf::ConnectedClientHeadlessServer
{
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

        // Need fullscreen for the cursor events
        auto const spec = mir_connection_create_spec_for_changes(connection);
        mir_surface_spec_set_fullscreen_on_output(spec, 1);
        mir_surface_apply_spec(surface1, spec);
        mir_surface_apply_spec(surface2, spec);
        mir_surface_spec_release(spec);
        
        bool surface_fullscreen = mt::spin_wait_for_condition_or_timeout(
            [this]
            {
                return mir_surface_get_state(surface1) == mir_surface_state_fullscreen &&
                       mir_surface_get_state(surface2) == mir_surface_state_fullscreen;
            },
            std::chrono::seconds{max_wait});

        EXPECT_TRUE(surface_fullscreen);

        mir_connection_set_lifecycle_event_callback(connection, lifecycle_changed, this);
    }

    MirSurface* surface1;
    MirSurface* surface2;

    std::vector<MirCookie> key_cookies;
    std::vector<MirCookie> pointer_cookies;

    MirLifecycleState lifecycle_state{mir_lifecycle_state_resumed};

    std::unique_ptr<mtf::FakeInputDevice> fake_keyboard{
        mtf::add_fake_input_device(mi::InputDeviceInfo{"keyboard", "keyboard-uid", mi::DeviceCapability::keyboard})
        };
    std::unique_ptr<mtf::FakeInputDevice> fake_pointer{
        mtf::add_fake_input_device(mi::InputDeviceInfo{"mouse", "mouse-uid", mi::DeviceCapability::pointer})
        };
};

namespace
{
// FIXME Removing the public API calls for the mir cookie, fix coming in 0.19
void cookie_capturing_callback(MirSurface* /*surface*/, MirEvent const* /*ev*/, void* /*ctx*/)
{
}

void lifecycle_changed(MirConnection* /*connection*/, MirLifecycleState state, void* ctx)
{
    auto client = reinterpret_cast<RaiseSurfaces*>(ctx);
    client->lifecycle_state = state;
}

bool wait_for_n_events(size_t n, std::vector<MirCookie>& cookies)
{
    bool all_events = mt::spin_wait_for_condition_or_timeout(
        [&n, &cookies]
        {
            return cookies.size() >= n;
        },
        std::chrono::seconds{max_wait});

   EXPECT_TRUE(all_events);
   return all_events;
}

}

// FIXME Removing the public API calls for the mir cookie, fix coming in 0.19
TEST_F(RaiseSurfaces, DISABLED_key_event_with_cookie)
{
    fake_keyboard->emit_event(mis::a_key_down_event().of_scancode(KEY_M));
    if (wait_for_n_events(1, key_cookies))
    {
        EXPECT_EQ(mir_surface_get_focus(surface2), mir_surface_focused);
        // EXPECT_TRUE attempt_focus surface2 key_cookies.back()
    }
}

// FIXME Removing the public API calls for the mir cookie, fix coming in 0.19
TEST_F(RaiseSurfaces, DISABLED_older_timestamp_does_not_focus)
{
    fake_keyboard->emit_event(mis::a_key_down_event().of_scancode(KEY_M));
    fake_keyboard->emit_event(mis::a_key_up_event().of_scancode(KEY_M));
    if (wait_for_n_events(2, key_cookies))
    {
        EXPECT_TRUE(key_cookies.front().timestamp < key_cookies.back().timestamp);
        EXPECT_EQ(mir_surface_get_focus(surface2), mir_surface_focused);

        // mir_surface_raise_with_cookie

        // Need to wait for this call to actually go through
        std::this_thread::sleep_for(std::chrono::milliseconds{1000});
        EXPECT_EQ(mir_surface_get_focus(surface2), mir_surface_focused);
    }
}

// FIXME Removing the public API calls for the mir cookie, fix coming in 0.19
TEST_F(RaiseSurfaces, DISABLED_motion_events_dont_prevent_raise)
{
    fake_keyboard->emit_event(mis::a_key_down_event().of_scancode(KEY_M));
    fake_keyboard->emit_event(mis::a_key_up_event().of_scancode(KEY_M));
    if (wait_for_n_events(2, key_cookies))
    {
        fake_pointer->emit_event(mis::a_pointer_event().with_movement(1, 1));
        if (wait_for_n_events(1, pointer_cookies))
        {
            EXPECT_EQ(mir_surface_get_focus(surface2), mir_surface_focused);
            // EXPECT_TRUE attempt_focus surface1 key_cookies.back()
        }
    }
}

// FIXME Removing the public API calls for the mir cookie, fix coming in 0.19
TEST_F(RaiseSurfaces, DISABLED_client_connection_close_invalid_cookie)
{
    // mir_surface_raise_with_cookie

    bool connection_close = mt::spin_wait_for_condition_or_timeout(
        [this]
        {
            return lifecycle_state == mir_lifecycle_connection_lost;
        },
        std::chrono::seconds{max_wait});

    EXPECT_TRUE(connection_close);
}
