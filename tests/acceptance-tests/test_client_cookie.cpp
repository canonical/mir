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
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 *              Brandon Schaefer <brandon.schaefer@canonical.com>
 */

#include "mir/input/input_device_info.h"
#include "mir_test_framework/fake_input_device.h"

#include "mir_test_framework/stub_server_platform_factory.h"
#include "mir_test_framework/connected_client_with_a_surface.h"
#include "mir/test/spin_wait.h"
#include "mir/cookie_factory.h"

#include "boost/throw_exception.hpp"

#include <linux/input.h>

namespace mtf = mir_test_framework;
namespace mt = mir::test;
namespace mi = mir::input;
namespace mis = mir::input::synthesis;

namespace
{
//std::chrono::seconds const max_wait{4};
//void cookie_capturing_callback(MirSurface*, MirEvent const* ev, void* ctx);
}
/*
class ClientCookies : public mtf::ConnectedClientWithASurface
{
public:
    ClientCookies()
    {
        server.override_the_cookie_factory([this] ()
            { return mir::cookie::CookieFactory::create_saving_secret(cookie_secret); });
    }

    void SetUp() override
    {
        mtf::ConnectedClientWithASurface::SetUp();

        // Need fullscreen for the cursor events
        auto const spec = mir_connection_create_spec_for_changes(connection);
        mir_surface_spec_set_fullscreen_on_output(spec, 1);
        mir_surface_apply_spec(surface, spec);
        mir_surface_spec_release(spec);

        mir_surface_set_event_handler(surface, &cookie_capturing_callback, this);
        mir_buffer_stream_swap_buffers_sync(mir_surface_get_buffer_stream(surface));
    }

    std::vector<uint8_t> cookie_secret;
    std::vector<MirCookie> out_cookies;

    std::unique_ptr<mtf::FakeInputDevice> fake_keyboard{
        mtf::add_fake_input_device(mi::InputDeviceInfo{"keyboard", "keyboard-uid" , mi::DeviceCapability::keyboard})
        };
    std::unique_ptr<mtf::FakeInputDevice> fake_pointer{
        mtf::add_fake_input_device(mi::InputDeviceInfo{"mouse", "mouse-uid" , mi::DeviceCapability::pointer})
        };
   std::unique_ptr<mtf::FakeInputDevice> fake_touch_screen{
       mtf::add_fake_input_device(mi::InputDeviceInfo{
       "touch screen", "touch-screen-uid", mi::DeviceCapability::touchscreen | mi::DeviceCapability::multitouch})
       };
};

namespace
{
// FIXME Removing the public API calls for the mir cookie, fix coming in 0.19
void cookie_capturing_callback(MirSurface*, MirEvent const* ev, void* ctx)
{
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
TEST_F(ClientCookies, DISABLED_keyboard_events_have_attestable_cookies)
{
    fake_keyboard->emit_event(mis::a_key_down_event().of_scancode(KEY_M));
    if (wait_for_n_events(1, out_cookies))
    {
        auto factory = mir::cookie::CookieFactory::create_from_secret(cookie_secret);
        EXPECT_TRUE(factory->attest_timestamp(out_cookies.back()));
    }
}

// FIXME Removing the public API calls for the mir cookie, fix coming in 0.19
TEST_F(ClientCookies, DISABLED_pointer_motion_events_do_not_have_attestable_cookies)
{
    fake_pointer->emit_event(mis::a_pointer_event().with_movement(1, 1));
    if (wait_for_n_events(1, out_cookies))
    {
        auto factory = mir::cookie::CookieFactory::create_from_secret(cookie_secret);
        EXPECT_FALSE(factory->attest_timestamp(out_cookies.back()));
    }
}

TEST_F(ClientCookies, DISABLED_pointer_click_events_have_attestable_cookies)
{
    fake_pointer->emit_event(mis::a_button_down_event().of_button(BTN_LEFT).with_action(mis::EventAction::Down));
    fake_pointer->emit_event(mis::a_button_up_event().of_button(BTN_LEFT));
    if (wait_for_n_events(2, out_cookies))
    {
        auto factory = mir::cookie::CookieFactory::create_from_secret(cookie_secret);
        EXPECT_TRUE(factory->attest_timestamp(out_cookies.back()));
    }
}

// FIXME Removing the public API calls for the mir cookie, fix coming in 0.19
TEST_F(ClientCookies, DISABLED_touch_motion_events_do_not_have_attestable_cookies)
{
    fake_touch_screen->emit_event(
         mis::a_touch_event()
        .at_position({0, 0})
        );

    fake_touch_screen->emit_event(
         mis::a_touch_event()
        .with_action(mis::TouchParameters::Action::Move)
        .at_position({1, 1})
        );

    if (wait_for_n_events(2, out_cookies))
    {
        auto factory = mir::cookie::CookieFactory::create_from_secret(cookie_secret);
        EXPECT_FALSE(factory->attest_timestamp(out_cookies.back()));
    }
}

// FIXME Removing the public API calls for the mir cookie, fix coming in 0.19
TEST_F(ClientCookies, DISABLED_touch_click_events_have_attestable_cookies)
{
    fake_touch_screen->emit_event(
         mis::a_touch_event()
        .at_position({0, 0})
        );

    if (wait_for_n_events(1, out_cookies))
    {
        auto factory = mir::cookie::CookieFactory::create_from_secret(cookie_secret);
        EXPECT_TRUE(factory->attest_timestamp(out_cookies.back()));
    }
}
*/
