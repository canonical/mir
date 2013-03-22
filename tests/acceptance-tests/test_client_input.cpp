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

#include "mir_toolkit/mir_client_library.h"
#include "mir_toolkit/input/event.h" // TODO: Remove once included in mir_client_library ~racarr

#include "mir_test/fake_shared.h"
#include "mir_test/fake_event_hub_input_configuration.h"
#include "mir_test/fake_event_hub.h"
#include "mir_test_framework/display_server_test_fixture.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mi = mir::input;
namespace mia = mi::android;
namespace mt = mir::test;
namespace mtd = mt::doubles;
namespace mtf = mir_test_framework;

namespace
{
    char const* const mir_test_socket = mtf::test_socket_file().c_str();
}

namespace
{

struct FakeInputServerConfiguration : public mir_test_framework::TestingServerConfiguration
{
    void exec(mir::DisplayServer* /* display_server */) override
    {
        std::shared_ptr<mi::CursorListener> null_cursor_listener;

        input_config = std::make_shared<mtd::FakeEventHubInputConfiguration>(the_event_filters(),
                                                                             the_display(),
                                                                             null_cursor_listener);
        fake_event_hub = input_config->the_fake_event_hub();

        fake_event_hub->synthesize_builtin_keyboard_added();
        fake_event_hub->synthesize_device_scan_complete();
    }

    std::shared_ptr<mi::InputManager>
    the_input_manager()
    {
        return input_manager(
        [this]() -> std::shared_ptr<mi::InputManager>
        {
            return std::make_shared<mia::InputManager>(input_config);
        });
    }

    std::shared_ptr<mtd::FakeEventHubInputConfiguration> input_config;
    mia::FakeEventHub* fake_event_hub;
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
    MOCK_METHOD1(handle_input, void(MirEvent *));
};

struct InputReceivingClient : ClientConfigCommon
{
    static void handle_input(MirSurface* surface, MirEvent* ev, void* context)
    {
        (void) surface;

        auto client = static_cast<InputReceivingClient *>(context);
        client->handler.handle_input(ev);
    }

    void exec()
    {
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
         mir_wait_for(mir_surface_create(connection, &request_params, create_surface_callback, this));
         mir_connection_release(connection);
    }
    
    MockInputHandler handler;
};

}

TEST_F(BespokeDisplayServerTestFixture, clients_receive_input)
{
    FakeInputServerConfiguration server_config;
    launch_server_process(server_config);

    InputReceivingClient client_config;
    launch_client_process(client_config);
}

