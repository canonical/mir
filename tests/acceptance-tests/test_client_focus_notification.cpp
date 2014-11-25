/*
 * Copyright © 2013-2014 Canonical Ltd.
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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#include "mir_toolkit/mir_client_library.h"

#include "mir_test/wait_condition.h"
#include "mir_test/event_matchers.h"

#include "mir_test_framework/interprocess_client_server_test.h"
#include "mir_test_framework/cross_process_sync.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mt = mir::test;
namespace mtf = mir_test_framework;

using namespace ::testing;

namespace
{
struct MockEventObserver
{
    MOCK_METHOD1(see, void(MirEvent const*));
};

struct ClientFocusNotification : mtf::InterprocessClientServerTest
{
    void SetUp() override
    {
        mtf::InterprocessClientServerTest::SetUp();
        run_in_server([]{});
    }

    MockEventObserver observer;
    mt::WaitCondition all_events_received;

    void connect_and_create_surface()
    {
        static int const surface_width = 100;
        static int const surface_height = 100;

        MirConnection *connection = mir_connect_sync(mir_test_socket, __PRETTY_FUNCTION__);
        ASSERT_TRUE(mir_connection_is_valid(connection));

        MirSurfaceParameters const request_params =
            {
                __PRETTY_FUNCTION__,
                surface_width, surface_height,
                mir_pixel_format_abgr_8888,
                mir_buffer_usage_hardware,
                mir_display_output_id_invalid
            };
        mir_wait_for(mir_connection_create_surface(connection, &request_params, surface_created, this));
        all_events_received.wait_for_at_most_seconds(60);
        mir_surface_release_sync(surface);
        mir_connection_release(connection);
    }

private:

    MirSurface *surface;

    static void handle_event(MirSurface* /* surface */, MirEvent const* ev, void* context)
    {
        auto self = static_cast<ClientFocusNotification*>(context);
        self->observer.see(ev);
    }

    static void surface_created(MirSurface *surface_, void *ctx)
    {
        auto self = static_cast<ClientFocusNotification*>(ctx);

        self->surface = surface_;
        // We need to set the event delegate from the surface_created
        // callback so we can block the reading of new events
        // until we are ready
        MirEventDelegate const event_delegate =
            {
                handle_event,
                self
            };
        mir_surface_set_event_handler(surface_, &event_delegate);
    }
};
}

TEST_F(ClientFocusNotification, a_surface_is_notified_of_receiving_focus)
{
    run_in_client([&]
        {
            EXPECT_CALL(observer, see(Pointee(mt::SurfaceEvent(mir_surface_attrib_focus, mir_surface_focused)))).Times(1)
                .WillOnce(mt::WakeUp(&all_events_received));
            // We may not see mir_surface_unfocused before connection closes
            EXPECT_CALL(observer, see(Pointee(mt::SurfaceEvent(mir_surface_attrib_focus, mir_surface_unfocused)))).Times(AtMost(1));

            connect_and_create_surface();
        });
}

namespace
{

ACTION_P(SignalFence, fence)
{
    fence->try_signal_ready_for();
}

}

TEST_F(ClientFocusNotification, two_surfaces_are_notified_of_gaining_and_losing_focus)
{
    // We use this for synchronization to ensure the two clients
    // are launched in a defined order.
    mtf::CrossProcessSync ready_for_second_client;

    auto const client_one = new_client_process([&]
        {
            InSequence seq;
            // We should receive focus as we are created
            EXPECT_CALL(observer, see(Pointee(mt::SurfaceEvent(mir_surface_attrib_focus,
                mir_surface_focused)))).Times(1)
                    .WillOnce(SignalFence(&ready_for_second_client));

            // And lose it as the second surface is created
            EXPECT_CALL(observer, see(Pointee(mt::SurfaceEvent(mir_surface_attrib_focus,
                mir_surface_unfocused)))).Times(1);
            // And regain it when the second surface is closed
            EXPECT_CALL(observer, see(Pointee(mt::SurfaceEvent(mir_surface_attrib_focus,
                mir_surface_focused)))).Times(1).WillOnce(mt::WakeUp(&all_events_received));
            // And then lose it as we are closed (but we may not see confirmation before connection closes)
            EXPECT_CALL(observer, see(Pointee(mt::SurfaceEvent(mir_surface_attrib_focus,
                mir_surface_unfocused)))).Times(AtMost(1));

            connect_and_create_surface();
        });

    auto const client_two = new_client_process([&]
        {
            ready_for_second_client.wait_for_signal_ready_for();

            EXPECT_CALL(observer, see(Pointee(
                mt::SurfaceEvent(mir_surface_attrib_focus, mir_surface_focused))))
                    .Times(1).WillOnce(mt::WakeUp(&all_events_received));
            // We may not see mir_surface_unfocused before connection closes
            EXPECT_CALL(observer, see(Pointee(
                mt::SurfaceEvent(mir_surface_attrib_focus, mir_surface_unfocused))))
                    .Times(AtMost(1));

            connect_and_create_surface();
        });
}
