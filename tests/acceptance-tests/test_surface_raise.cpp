/*
 * Copyright © 2015-2016 Canonical Ltd.
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
#include "mir/cookie_authority.h"

#include "mir_toolkit/mir_client_library.h"

#include <linux/input.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <mutex>

namespace mtf = mir_test_framework;
namespace mt = mir::test;
namespace mi = mir::input;
namespace mis = mir::input::synthesis;

namespace
{
std::chrono::seconds const max_wait{4};
void cookie_capturing_callback(MirSurface* surface, MirEvent const* ev, void* ctx);
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
    }

    MirSurface* surface1;
    MirSurface* surface2;

    std::vector<std::vector<uint8_t>> out_cookies;
    size_t event_count{0};
    mutable std::mutex mutex;

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

void cookie_capturing_callback(MirSurface* /*surface*/, MirEvent const* ev, void* ctx)
{
    auto const event_type = mir_event_get_type(ev);
    auto raise_surfaces = static_cast<RaiseSurfaces*>(ctx);
    
    if (event_type == mir_event_type_input)
    {   
        auto const* iev = mir_event_get_input_event(ev);

        std::lock_guard<std::mutex> lk(raise_surfaces->mutex);
        if (mir_input_event_has_cookie(iev))
        {
            auto cookie = mir_input_event_get_cookie(iev);
            size_t size = mir_cookie_get_size(cookie);

            std::vector<uint8_t> cookie_bytes(size);
            mir_cookie_copy_to_buffer(cookie, cookie_bytes.data(), size);

            mir_cookie_release(cookie);

            raise_surfaces->out_cookies.push_back(cookie_bytes);
        }
        
        raise_surfaces->event_count++;
    }
}

bool wait_for_n_events(size_t n, RaiseSurfaces const* raise_surfaces)
{
    bool all_events = mt::spin_wait_for_condition_or_timeout(
        [&n, &raise_surfaces]
        {
            std::lock_guard<std::mutex> lk(raise_surfaces->mutex);
            return raise_surfaces->event_count >= n;
        },
        std::chrono::seconds{max_wait});

   EXPECT_TRUE(all_events);
   return all_events;
}

bool attempt_focus(MirSurface* surface, MirCookie const* cookie)
{
    mir_surface_raise_with_cookie(surface, cookie);
    bool surface_becomes_focused = mt::spin_wait_for_condition_or_timeout(
        [&surface]
        {
            return mir_surface_get_focus(surface) == mir_surface_focused;
        },
        std::chrono::seconds{max_wait});
 
    return surface_becomes_focused;
}

}

TEST_F(RaiseSurfaces, key_event_with_cookie)
{
    fake_keyboard->emit_event(mis::a_key_down_event().of_scancode(KEY_M));

    int events = 1;
    if (wait_for_n_events(events, this))
    {
        std::lock_guard<std::mutex> lk(mutex);
        ASSERT_FALSE(out_cookies.empty());
        EXPECT_EQ(mir_surface_get_focus(surface2), mir_surface_focused);

        MirCookie const* cookie = mir_cookie_from_buffer(out_cookies.back().data(), out_cookies.back().size());
        attempt_focus(surface2, cookie);

        mir_cookie_release(cookie);
    }
}

TEST_F(RaiseSurfaces, older_timestamp_does_not_focus)
{
    fake_keyboard->emit_event(mis::a_key_down_event().of_scancode(KEY_M));
    fake_keyboard->emit_event(mis::a_key_up_event().of_scancode(KEY_M));

    int events = 2;
    if (wait_for_n_events(events, this))
    {
        std::lock_guard<std::mutex> lk(mutex);
        ASSERT_FALSE(out_cookies.empty());
        EXPECT_EQ(mir_surface_get_focus(surface2), mir_surface_focused);

        MirCookie const* cookie = mir_cookie_from_buffer(out_cookies.front().data(), out_cookies.front().size());
        mir_surface_raise_with_cookie(surface1, cookie);

        mir_cookie_release(cookie);

        // Need to wait for this call to actually go through
        std::this_thread::sleep_for(std::chrono::milliseconds{1000});
        EXPECT_EQ(mir_surface_get_focus(surface2), mir_surface_focused);
    }
}

TEST_F(RaiseSurfaces, motion_events_dont_prevent_raise)
{
    fake_keyboard->emit_event(mis::a_key_down_event().of_scancode(KEY_M));
    fake_keyboard->emit_event(mis::a_key_up_event().of_scancode(KEY_M));
    if (wait_for_n_events(2, this))
    {
        fake_pointer->emit_event(mis::a_pointer_event().with_movement(1, 1));
        if (wait_for_n_events(1, this))
        {
            std::lock_guard<std::mutex> lk(mutex);
            ASSERT_FALSE(out_cookies.empty());
            EXPECT_EQ(mir_surface_get_focus(surface2), mir_surface_focused);
            MirCookie const* cookie = mir_cookie_from_buffer(out_cookies.back().data(), out_cookies.back().size());
            EXPECT_TRUE(attempt_focus(surface1, cookie));
            mir_cookie_release(cookie);
        }
    }
}
