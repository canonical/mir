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

#include "mir/graphics/display.h"

#include "src/server/input/android/android_input_manager.h"
#include "src/server/input/android/android_dispatcher_controller.h"

#include "mir_toolkit/mir_client_library.h"

#include "mir_test/fake_shared.h"
#include "mir_test/fake_event_hub_input_configuration.h"
#include "mir_test/fake_event_hub.h"
#include "mir_test/event_factory.h"
#include "mir_test/wait_condition.h"
#include "mir_test_framework/display_server_test_fixture.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <thread>
#include <functional>

namespace mi = mir::input;
namespace mia = mi::android;
namespace mis = mi::synthesis;
namespace msh = mir::shell;
namespace mt = mir::test;
namespace mtd = mt::doubles;
namespace mtf = mir_test_framework;

namespace
{
    char const* const mir_test_socket = mtf::test_socket_file().c_str();
}

namespace
{

struct FocusNotifyingDispatcherController : public mia::DispatcherController
{
    FocusNotifyingDispatcherController(std::shared_ptr<mia::InputConfiguration> const& configuration,
                                       mt::WaitCondition &wait_condition)
      : DispatcherController(configuration),
        on_focus_set(wait_condition)
    {

    }
    
    void set_input_focus_to(
        std::shared_ptr<mi::SessionTarget> const& session, std::shared_ptr<mi::SurfaceTarget> const& surface) override
    {
        DispatcherController::set_input_focus_to(session, surface);
        
        // We need a synchronization primitive inorder to halt test event injection
        // until after a surface has taken focus (lest the events be discarded).
        if (surface)
            on_focus_set.wake_up_everyone();
    }

    mt::WaitCondition &on_focus_set;
};

struct FakeInputServerConfiguration : public mir_test_framework::TestingServerConfiguration
{
    FakeInputServerConfiguration()
      : input_config(the_event_filters(), the_display(), std::shared_ptr<mi::CursorListener>())
    {
    }

    virtual void inject_input()
    {
    }
    
    void exec() override
    {
        input_injection_thread = std::thread([this]() -> void
        {
            on_focus_set.wait_for_at_most_seconds(10);
            fake_event_hub->synthesize_builtin_keyboard_added();
            fake_event_hub->synthesize_device_scan_complete();
            inject_input();
        });
    }
    
    void on_exit() override
    {
        input_injection_thread.join();
    }
    
    std::shared_ptr<mia::InputConfiguration> the_input_configuration() override
    {
        fake_event_hub = input_config.the_fake_event_hub();
        return mt::fake_shared(input_config);
    }
    
    std::shared_ptr<mi::InputManager> the_input_manager() override
    {
        return input_manager(
            [&]()
            {
                // Force usage of real input even in case of tests-use-real-input = false.
                return std::make_shared<mia::InputManager>(the_input_configuration());
            });
    }

    std::shared_ptr<msh::InputFocusSelector>
    the_input_focus_selector() override
    {
        return input_focus_selector(
            [this]()
            {
                return std::make_shared<FocusNotifyingDispatcherController>(mt::fake_shared(input_config), on_focus_set);
            });
    }

    mtd::FakeEventHubInputConfiguration input_config;
    mia::FakeEventHub* fake_event_hub;
    mt::WaitCondition on_focus_set;
    std::thread input_injection_thread;
};


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
        surface = NULL;
    }

    MirConnection* connection;
    MirSurface* surface;
};

struct MockInputHandler
{
    MOCK_METHOD1(handle_input, void(MirEvent const*));
};

struct InputReceivingClient : ClientConfigCommon
{
    InputReceivingClient()
      : events_received(0)
    {
    }

    static void handle_input(MirSurface* /* surface */, MirEvent const* ev, void* context)
    {
        auto client = static_cast<InputReceivingClient *>(context);
        client->handler->handle_input(ev);
        client->event_received[client->events_received].wake_up_everyone();
        client->events_received++;
    }
    virtual void expect_input()
    {
    }

    void exec()
    {
        handler = std::make_shared<MockInputHandler>();

        expect_input();

        mir_wait_for(mir_connect(
            mir_test_socket,
            __PRETTY_FUNCTION__,
            connection_callback,
            this));
         ASSERT_TRUE(connection != NULL);
         MirSurfaceParameters const request_params =
         {
             __PRETTY_FUNCTION__,
             640, 480,
             mir_pixel_format_abgr_8888,
             mir_buffer_usage_hardware
         };
         MirEventDelegate const event_delegate =
         {
             handle_input,
             this
         };
         mir_wait_for(mir_surface_create(connection, &request_params, create_surface_callback, this));

         mir_surface_set_event_handler(surface, &event_delegate);

         event_received[0].wait_for_at_most_seconds(5);
         event_received[1].wait_for_at_most_seconds(5);
         event_received[2].wait_for_at_most_seconds(5);

         mir_surface_release_sync(surface);
         
         mir_connection_release(connection);

         // ClientConfiguration d'tor is not called on client side so we need this
         // in order to not leak the Mock object.
         handler.reset(); 
    }
    
    std::shared_ptr<MockInputHandler> handler;
    mt::WaitCondition event_received[3];
    int events_received;
};

}

using TestClientInput = BespokeDisplayServerTestFixture;

TEST_F(TestClientInput, clients_receive_key_input)
{
    using namespace ::testing;
    
    int const num_events_produced = 3;

    struct InputProducingServerConfiguration : FakeInputServerConfiguration
    {
        void inject_input()
        {
            // We send multiple events in order to verify the client is handling them
            // and server dispatch does not become backed up.
            for (int i = 0; i < num_events_produced; i++)
                fake_event_hub->synthesize_event(mis::a_key_down_event()
                                                 .of_scancode(KEY_ENTER));
        }
    } server_config;
    launch_server_process(server_config);
    
    struct KeyReceivingClient : InputReceivingClient
    {
        void expect_input()
        {
            using namespace ::testing;
            EXPECT_CALL(*handler, handle_input(_)).Times(num_events_produced);
        }
    } client_config;
    launch_client_process(client_config);
}

