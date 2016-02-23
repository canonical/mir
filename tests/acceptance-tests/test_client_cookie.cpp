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
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 *              Brandon Schaefer <brandon.schaefer@canonical.com>
 */

#include "mir/input/input_device_info.h"
#include "mir_test_framework/fake_input_device.h"

#include "mir_test_framework/stub_server_platform_factory.h"
#include "mir_test_framework/connected_client_with_a_surface.h"
#include "mir/test/spin_wait.h"
#include "mir/test/wait_condition.h"
#include "mir/cookie/authority.h"
#include "mir_test_framework/visible_surface.h"

#include "boost/throw_exception.hpp"

#include <linux/input.h>

#include <mutex>

namespace mtf = mir_test_framework;
namespace mt = mir::test;
namespace mi = mir::input;
namespace mis = mir::input::synthesis;

namespace
{
std::chrono::seconds const max_wait{4};
void cookie_capturing_callback(MirSurface*, MirEvent const* ev, void* ctx);
}

class ClientCookies : public mtf::ConnectedClientHeadlessServer
{
public:
    ClientCookies()
    {
        server.override_the_cookie_authority([this] ()
            { return mir::cookie::Authority::create_saving(cookie_secret); });
    }

    void SetUp() override
    {
        mtf::ConnectedClientHeadlessServer::SetUp();

        // Need fullscreen for the cursor events
        auto const spec = mir_connection_create_spec_for_normal_surface(
            connection, 100, 100, mir_pixel_format_abgr_8888);
        mir_surface_spec_set_fullscreen_on_output(spec, 1);
        mir_surface_spec_set_event_handler(spec, &cookie_capturing_callback, this);
        surface = mir_surface_create_sync(spec);
        mir_surface_spec_release(spec);

        mir_buffer_stream_swap_buffers_sync(mir_surface_get_buffer_stream(surface));

        ready_to_accept_events.wait_for_at_most_seconds(max_wait);
        if (!ready_to_accept_events.woken())
            BOOST_THROW_EXCEPTION(std::runtime_error("Timeout waiting for surface to become focused and exposed"));
    }

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

    mir::test::WaitCondition ready_to_accept_events;
    std::vector<std::vector<uint8_t>> out_cookies;
    std::vector<uint8_t> cookie_secret;
    size_t event_count{0};
    mutable std::mutex mutex;
    bool exposed{false};
    bool focused{false};
    MirSurface* surface;
};

namespace
{

void cookie_capturing_callback(MirSurface*, MirEvent const* ev, void* ctx)
{
    auto const event_type = mir_event_get_type(ev);
    auto client_cookie = static_cast<ClientCookies*>(ctx);

    if (event_type == mir_event_type_surface)
    {
        auto event = mir_event_get_surface_event(ev);
        auto const attrib = mir_surface_event_get_attribute(event);
        auto const value = mir_surface_event_get_attribute_value(event);

        std::lock_guard<std::mutex> lk(client_cookie->mutex);
        if (attrib == mir_surface_attrib_visibility &&
            value == mir_surface_visibility_exposed)
        {
            client_cookie->exposed = true;
        }

        if (attrib == mir_surface_attrib_focus &&
            value == mir_surface_focused)
        {
            client_cookie->focused = true;
        }

        if (client_cookie->exposed && client_cookie->focused)
            client_cookie->ready_to_accept_events.wake_up_everyone();
    }
    else if (event_type == mir_event_type_input)
    {
        auto const* iev = mir_event_get_input_event(ev);

        std::lock_guard<std::mutex> lk(client_cookie->mutex);
        if (mir_input_event_has_cookie(iev))
        {
            auto cookie = mir_input_event_get_cookie(iev);
            size_t size = mir_cookie_buffer_size(cookie);

            std::vector<uint8_t> cookie_bytes(size);
            mir_cookie_to_buffer(cookie, cookie_bytes.data(), size);

            mir_cookie_release(cookie);
            client_cookie->out_cookies.push_back(cookie_bytes);
        }

        client_cookie->event_count++;
    }
}

bool wait_for_n_events(size_t n, ClientCookies* client_cookie)
{
    bool all_events = mt::spin_wait_for_condition_or_timeout(
        [&n, &client_cookie]
        {
            std::lock_guard<std::mutex> lk(client_cookie->mutex);
            return client_cookie->event_count >= n;
        },
        std::chrono::seconds{max_wait});

   EXPECT_TRUE(all_events);
   return all_events;
}
}

TEST_F(ClientCookies, keyboard_events_have_cookies)
{
    fake_keyboard->emit_event(mis::a_key_down_event().of_scancode(KEY_M));

    int events = 1;
    if (wait_for_n_events(events, this))
    {
        std::lock_guard<std::mutex> lk(mutex);

        ASSERT_FALSE(out_cookies.empty());
        auto authority = mir::cookie::Authority::create_from(cookie_secret);

        EXPECT_NO_THROW(authority->make_cookie(out_cookies.back()));
    }
}

TEST_F(ClientCookies, pointer_motion_events_do_not_have_cookies)
{
    // with movement generates 2 events
    fake_pointer->emit_event(mis::a_pointer_event().with_movement(1, 1));

    int events = 2;
    if (wait_for_n_events(events, this))
    {
        std::lock_guard<std::mutex> lk(mutex);
        EXPECT_EQ(event_count, events);
        EXPECT_TRUE(out_cookies.empty());
    }
}

TEST_F(ClientCookies, pointer_click_events_have_cookies)
{
    fake_pointer->emit_event(mis::a_button_down_event().of_button(BTN_LEFT).with_action(mis::EventAction::Down));
    fake_pointer->emit_event(mis::a_button_up_event().of_button(BTN_LEFT));

    int events = 2;
    if (wait_for_n_events(events, this))
    {
        std::lock_guard<std::mutex> lk(mutex);
        ASSERT_FALSE(out_cookies.empty());
        auto authority = mir::cookie::Authority::create_from(cookie_secret);
        EXPECT_NO_THROW(authority->make_cookie(out_cookies.back()));
    }
}

TEST_F(ClientCookies, touch_motion_events_do_not_have_cookies)
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

    int events = 1;
    if (wait_for_n_events(events, this))
    {
        std::lock_guard<std::mutex> lk(mutex);
        EXPECT_GE(event_count, events);
        EXPECT_EQ(out_cookies.size(), 1);
    }
}
