/*
 * Copyright © 2013 Canonical Ltd.
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

#include "mir_test_framework/display_server_test_fixture.h"
#include "mir_test_framework/cross_process_sync.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mt = mir::test;
namespace mtf = mir_test_framework;

namespace
{
    char const* const mir_test_socket = mtf::test_socket_file().c_str();
}

namespace
{
struct ClientConfigCommon : TestingClientConfiguration
{
    ClientConfigCommon() :
        connection(0),
        surface(0)
    {
    }
    static void connection_callback(MirConnection* connection, void* context)
    {
        ClientConfigCommon* config = reinterpret_cast<ClientConfigCommon *>(context);
        config->connection = connection;
    }
    static void create_surface_callback(MirSurface* surface, void* context)
    {
        ClientConfigCommon* config = reinterpret_cast<ClientConfigCommon *>(context);
        config->surface_created(surface);
    }
    static void release_surface_callback(MirSurface* surface, void* context)
    {
        ClientConfigCommon* config = reinterpret_cast<ClientConfigCommon *>(context);
        config->surface_released(surface);
    }
    virtual void connected(MirConnection* new_connection)
    {
        connection = new_connection;
    }
    virtual void surface_created(MirSurface* new_surface)
    {
        surface = new_surface;
    }
    virtual void surface_released(MirSurface* /* released_surface */)
    {
        surface = nullptr;
    }
    MirConnection* connection;
    MirSurface* surface;
};

struct MockEventObserver
{
    MOCK_METHOD1(see, void(MirEvent const*));
};

struct EventObservingClient : ClientConfigCommon
{
    EventObservingClient()
        : observer(std::make_shared<MockEventObserver>())
    {
    }

    static void handle_event(MirSurface* /* surface */, MirEvent const* ev, void* context)
    {
        auto client = static_cast<EventObservingClient *>(context);
        client->observer->see(ev);
    }
 
    virtual void expect_events(mt::WaitCondition* /* all_events_received */) = 0;

    void surface_created(MirSurface *surface_) override
    {
        // We need to set the event delegate from the surface_created
        // callback so we can block the reading of new events
        // until we are ready
        MirEventDelegate const event_delegate =
            {
                handle_event,
                this
            };
        surface = surface_;
        mir_surface_set_event_handler(surface, &event_delegate);
    }

    void exec()
    {
        mt::WaitCondition all_events_received;
        expect_events(&all_events_received);
        mir_wait_for(mir_connect(mir_test_socket,
            __PRETTY_FUNCTION__, connection_callback, this));
        ASSERT_TRUE(connection != NULL);
        MirSurfaceParameters const request_params =
            {
                __PRETTY_FUNCTION__,
                surface_width, surface_height,
                mir_pixel_format_abgr_8888,
                mir_buffer_usage_hardware,
                mir_display_output_id_invalid
            };
        mir_wait_for(mir_connection_create_surface(connection, &request_params, create_surface_callback, this));
        all_events_received.wait_for_at_most_seconds(60);
        mir_surface_release_sync(surface);
        mir_connection_release(connection);

        // The ClientConfig is not destroyed before the testing process 
        // exits.
        observer.reset();
    }
    std::shared_ptr<MockEventObserver> observer;
    static int const surface_width = 100;
    static int const surface_height = 100;
};

}

TEST_F(BespokeDisplayServerTestFixture, a_surface_is_notified_of_receiving_focus)
{
    using namespace ::testing;

    TestingServerConfiguration server_config;
    launch_server_process(server_config);

    struct FocusObservingClient : public EventObservingClient
    {
        void expect_events(mt::WaitCondition* all_events_received) override
        {
            EXPECT_CALL(*observer, see(Pointee(mt::SurfaceEvent(mir_surface_attrib_focus, mir_surface_focused)))).Times(1)
                .WillOnce(mt::WakeUp(all_events_received));
        }
    } client_config;
    launch_client_process(client_config);
}

namespace
{

ACTION_P(SignalFence, fence)
{
    fence->try_signal_ready_for();
}

}

TEST_F(BespokeDisplayServerTestFixture, two_surfaces_are_notified_of_gaining_and_losing_focus)
{
    using namespace ::testing;
    
    TestingServerConfiguration server_config;
    launch_server_process(server_config);

    // We use this for synchronization to ensure the two clients
    // are launched in a defined order.
    mtf::CrossProcessSync ready_for_second_client;

    struct FocusObservingClientOne : public EventObservingClient
    {
        mtf::CrossProcessSync ready_for_second_client;
        FocusObservingClientOne(mtf::CrossProcessSync const& ready_for_second_client)
            : ready_for_second_client(ready_for_second_client)
        {
        }

        void expect_events(mt::WaitCondition* all_events_received) override
        {
            InSequence seq;
            // We should receive focus as we are created
            EXPECT_CALL(*observer, see(Pointee(mt::SurfaceEvent(mir_surface_attrib_focus,
                mir_surface_focused)))).Times(1)
                    .WillOnce(SignalFence(&ready_for_second_client));

            // And lose it as the second surface is created
            EXPECT_CALL(*observer, see(Pointee(mt::SurfaceEvent(mir_surface_attrib_focus,
                mir_surface_unfocused)))).Times(1);
            // And regain it when the second surface is closed
            EXPECT_CALL(*observer, see(Pointee(mt::SurfaceEvent(mir_surface_attrib_focus,
                mir_surface_focused)))).Times(1).WillOnce(mt::WakeUp(all_events_received));
        }

    } client_one_config(ready_for_second_client);
    launch_client_process(client_one_config);

    struct FocusObservingClientTwo : public EventObservingClient
    {
        mtf::CrossProcessSync ready_for_second_client;
        FocusObservingClientTwo(mtf::CrossProcessSync const& ready_for_second_client)
            : ready_for_second_client(ready_for_second_client)
        {
        }
        void exec() override
        {
            // We need some synchronization to ensure client two does not connect before client one.
            ready_for_second_client.wait_for_signal_ready_for();
            EventObservingClient::exec();
        }

        void expect_events(mt::WaitCondition* all_events_received) override
        {
            EXPECT_CALL(*observer, see(Pointee(
                mt::SurfaceEvent(mir_surface_attrib_focus, mir_surface_focused))))
                    .Times(1).WillOnce(mt::WakeUp(all_events_received));
        }
    } client_two_config(ready_for_second_client);

    launch_client_process(client_two_config);
}
