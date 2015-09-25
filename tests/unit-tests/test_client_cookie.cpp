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
 */

#include "mir_test_framework/udev_environment.h"
#include "mir_test_framework/connected_client_with_a_surface.h"
#include "mir/cookie_factory.h"
#include "mir/test/wait_condition.h"

#include "boost/throw_exception.hpp"

namespace mtf = mir_test_framework;
namespace msh = mir::shell;

namespace
{
int const max_wait{4};
void cookie_capturing_callback(MirSurface*, MirEvent const* ev, void* ctx);
}

class ClientCookies : public mtf::ConnectedClientWithASurface
{
public:
    ClientCookies()
        : out_cookie{0, 0}
    {
        add_to_environment("MIR_SERVER_PLATFORM_INPUT_LIB", nullptr);
        server.override_the_cookie_factory([this] ()
            { return mir::cookie::CookieFactory::create_saving_secret(cookie_secret); });

        mock_devices.add_standard_device("laptop-keyboard");
    }

    void SetUp() override
    {
        mtf::ConnectedClientWithASurface::SetUp();

        mir_surface_set_event_handler(surface, &cookie_capturing_callback, this);
        mir_buffer_stream_swap_buffers_sync(mir_surface_get_buffer_stream(surface));
    }

    void TearDown() override
    {
        mtf::ConnectedClientWithASurface::TearDown();
    }

    std::vector<uint8_t> cookie_secret;
    mtf::UdevEnvironment mock_devices;
    MirCookie out_cookie;
    mir::test::WaitCondition udev_read_recording;
};

namespace
{
void cookie_capturing_callback(MirSurface*, MirEvent const* ev, void* ctx)
{
    auto client = reinterpret_cast<ClientCookies*>(ctx);
    auto etype = mir_event_get_type(ev);
    if (etype == mir_event_type_input)
    {
        auto iev = mir_event_get_input_event(ev);
        auto itype = mir_input_event_get_type(iev);
        if (itype == mir_input_event_type_key)
        {
            auto kev = mir_input_event_get_keyboard_event(iev);
            client->out_cookie = mir_keyboard_event_get_cookie(kev);
            client->udev_read_recording.wake_up_everyone();
        }
    }
}
}

TEST_F(ClientCookies, keyboard_events_have_attestable_cookies)
{
    mock_devices.load_device_evemu("laptop-keyboard-hello");

    udev_read_recording.wait_for_at_most_seconds(max_wait);
    if (!udev_read_recording.woken())
        BOOST_THROW_EXCEPTION(std::runtime_error("Timeout waiting for udev to read the recording 'laptop-keyboard-hello'"));

    auto factory = mir::cookie::CookieFactory::create_from_secret(cookie_secret);

    EXPECT_TRUE(factory->attest_timestamp(out_cookie));
}
