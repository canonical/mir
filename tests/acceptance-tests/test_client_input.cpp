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
#include "mir/graphics/viewable_area.h"
#include "mir/shell/surface_creation_parameters.h"
#include "mir/shell/placement_strategy.h"

#include "src/server/input/android/android_input_manager.h"
#include "src/server/input/android/android_input_targeter.h"

#include "mir_toolkit/mir_client_library.h"

#include "mir_test/fake_shared.h"
#include "mir_test/fake_event_hub.h"
#include "mir_test/event_factory.h"
#include "mir_test/wait_condition.h"
#include "mir_test_framework/display_server_test_fixture.h"
#include "mir_test_framework/input_testing_server_configuration.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <xkbcommon/xkbcommon.h>
#include <xkbcommon/xkbcommon-keysyms.h>

#include <thread>
#include <functional>
#include <map>

namespace mi = mir::input;
namespace mia = mi::android;
namespace mis = mi::synthesis;
namespace mf = mir::frontend;
namespace msh = mir::shell;
namespace ms = mir::surfaces;
namespace mg = mir::graphics;
namespace geom = mir::geometry;
namespace mt = mir::test;
namespace mtd = mt::doubles;
namespace mtf = mir_test_framework;

namespace
{
    char const* const mir_test_socket = mtf::test_socket_file().c_str();
}

namespace
{

struct ClientConfigCommon : mtf::TestingClientConfiguration
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
    InputReceivingClient(std::string const& surface_name)
      : surface_name(surface_name)
    {
    }

    static void handle_input(MirSurface* /* surface */, MirEvent const* ev, void* context)
    {
        auto client = static_cast<InputReceivingClient *>(context);

        client->handler->handle_input(ev);
    }

    virtual void expect_input(mt::WaitCondition&)
    {
    }
    
    virtual MirSurfaceParameters parameters()
    {
        MirSurfaceParameters const request_params =
         {
             surface_name.c_str(),
             surface_width, surface_height,
             mir_pixel_format_abgr_8888,
             mir_buffer_usage_hardware
         };
        return request_params;
    }

    void exec()
    {
        handler = std::make_shared<MockInputHandler>();

        expect_input(events_received);

        mir_wait_for(mir_connect(
            mir_test_socket,
            __PRETTY_FUNCTION__,
            connection_callback,
            this));
         ASSERT_TRUE(connection != NULL);
    
         MirEventDelegate const event_delegate =
         {
             handle_input,
             this
         };
         auto request_params = parameters();
         mir_wait_for(mir_connection_create_surface(connection, &request_params, create_surface_callback, this));

         mir_surface_set_event_handler(surface, &event_delegate);

         events_received.wait_for_at_most_seconds(60);

         mir_surface_release_sync(surface);
         
         mir_connection_release(connection);

         // ClientConfiguration d'tor is not called on client side so we need this
         // in order to not leak the Mock object.
         handler.reset(); 
    }
    
    std::shared_ptr<MockInputHandler> handler;
    mt::WaitCondition events_received;
    
    std::string const surface_name;
    
    static int const surface_width = 100;
    static int const surface_height = 100;
};

MATCHER(KeyDownEvent, "")
{
    if (arg->type != mir_event_type_key)
        return false;
    if (arg->key.action != mir_key_action_down) // Key down
        return false;
    
    return true;
}
MATCHER_P(KeyOfSymbol, keysym, "")
{
    if (static_cast<xkb_keysym_t>(arg->key.key_code) == (uint)keysym)
        return true;
    return false;
}

MATCHER(HoverEnterEvent, "")
{
    if (arg->type != mir_event_type_motion)
        return false;
    if (arg->motion.action != mir_motion_action_hover_enter)
        return false;

    return true;
}
MATCHER(HoverExitEvent, "")
{
    if (arg->type != mir_event_type_motion)
        return false;
    if (arg->motion.action != mir_motion_action_hover_exit)
        return false;

    return true;
}

MATCHER_P2(ButtonDownEvent, x, y, "")
{
    if (arg->type != mir_event_type_motion)
        return false;
    if (arg->motion.action != mir_motion_action_down)
        return false;
    if (arg->motion.button_state == 0)
        return false;
    if (arg->motion.pointer_coordinates[0].x != x)
        return false;
    if (arg->motion.pointer_coordinates[0].y != y)
        return false;
    return true;
}

MATCHER_P2(MotionEventWithPosition, x, y, "")
{
    if (arg->type != mir_event_type_motion)
        return false;
    if (arg->motion.action != mir_motion_action_move &&
        arg->motion.action != mir_motion_action_hover_move)
        return false;
    if (arg->motion.pointer_coordinates[0].x != x)
        return false;
    if (arg->motion.pointer_coordinates[0].y != y)
        return false;
    return true;
}

}


using TestClientInput = BespokeDisplayServerTestFixture;

TEST_F(TestClientInput, clients_receive_key_input)
{
    using namespace ::testing;
    
    int const num_events_produced = 3;
    static std::string const test_client_name = "1";

    struct ServerConfiguration : mtf::InputTestingServerConfiguration
    {
        void inject_input()
        {
            wait_until_client_appears(test_client_name);
            for (int i = 0; i < num_events_produced; i++)
                fake_event_hub->synthesize_event(mis::a_key_down_event()
                                                 .of_scancode(KEY_ENTER));
        }
    } server_config;
    launch_server_process(server_config);
    
    struct KeyReceivingClient : InputReceivingClient
    {
        KeyReceivingClient() : InputReceivingClient(test_client_name) {}
        void expect_input(mt::WaitCondition& events_received) override
        {
            using namespace ::testing;
            InSequence seq;

            EXPECT_CALL(*handler, handle_input(KeyDownEvent())).Times(2);
            EXPECT_CALL(*handler, handle_input(KeyDownEvent())).Times(1)
                .WillOnce(mt::WakeUp(&events_received));
        }
    } client_config;
    launch_client_process(client_config);
}

TEST_F(TestClientInput, clients_receive_us_english_mapped_keys)
{
    using namespace ::testing;
    static std::string const test_client_name = "1";
    
    struct ServerConfiguration : mtf::InputTestingServerConfiguration
    {
        void inject_input()
        {
            wait_until_client_appears(test_client_name);

            fake_event_hub->synthesize_event(mis::a_key_down_event()
                                             .of_scancode(KEY_LEFTSHIFT));
            fake_event_hub->synthesize_event(mis::a_key_down_event()
                                             .of_scancode(KEY_4));

        }
    } server_config;
    launch_server_process(server_config);
    
    struct KeyReceivingClient : InputReceivingClient
    {
        KeyReceivingClient() : InputReceivingClient(test_client_name) {}

        void expect_input(mt::WaitCondition& events_received) override
        {
            using namespace ::testing;

            InSequence seq;
            EXPECT_CALL(*handler, handle_input(AllOf(KeyDownEvent(), KeyOfSymbol(XKB_KEY_Shift_L)))).Times(1);
            EXPECT_CALL(*handler, handle_input(AllOf(KeyDownEvent(), KeyOfSymbol(XKB_KEY_dollar)))).Times(1)
                .WillOnce(mt::WakeUp(&events_received));
        }
    } client_config;
    launch_client_process(client_config);
}

TEST_F(TestClientInput, clients_receive_motion_inside_window)
{
    using namespace ::testing;
    static std::string const test_client_name = "1";
    
    struct ServerConfiguration : public mtf::InputTestingServerConfiguration
    {
        void inject_input()
        {
            wait_until_client_appears(test_client_name);
            
            fake_event_hub->synthesize_event(mis::a_motion_event().with_movement(InputReceivingClient::surface_width,
                                                                                 InputReceivingClient::surface_height));
            fake_event_hub->synthesize_event(mis::a_motion_event().with_movement(2,2));
        }
    } server_config;
    launch_server_process(server_config);
    
    struct MotionReceivingClient : InputReceivingClient
    {
        MotionReceivingClient() : InputReceivingClient(test_client_name) {}

        void expect_input(mt::WaitCondition& events_received) override
        {
            using namespace ::testing;
            
            InSequence seq;

            // We should see the cursor enter
            EXPECT_CALL(*handler, handle_input(HoverEnterEvent())).Times(1);
            EXPECT_CALL(*handler, handle_input(
                MotionEventWithPosition(InputReceivingClient::surface_width, 
                                        InputReceivingClient::surface_height))).Times(1)
                .WillOnce(mt::WakeUp(&events_received));
            // But we should not receive an event for the second movement outside of our surface!
        }
    } client_config;
    launch_client_process(client_config);
}

TEST_F(TestClientInput, clients_receive_button_events_inside_window)
{
    using namespace ::testing;
    
    static std::string const test_client_name = "1";
    
    struct ServerConfiguration : public mtf::InputTestingServerConfiguration
    {
        void inject_input()
        {
            wait_until_client_appears(test_client_name);
            fake_event_hub->synthesize_event(mis::a_button_down_event().of_button(BTN_LEFT).with_action(mis::EventAction::Down));
        }
    } server_config;
    launch_server_process(server_config);
    
    struct ButtonReceivingClient : InputReceivingClient
    {
        ButtonReceivingClient() : InputReceivingClient(test_client_name) {}

        void expect_input(mt::WaitCondition& events_received) override
        {
            using namespace ::testing;
            
            InSequence seq;

            // The cursor starts at (0, 0).
            EXPECT_CALL(*handler, handle_input(ButtonDownEvent(0, 0))).Times(1)
                .WillOnce(mt::WakeUp(&events_received));
        }
    } client_config;
    launch_client_process(client_config);
}

namespace
{
typedef std::map<std::string, geom::Rectangle> GeometryList;

struct StaticPlacementStrategy : public msh::PlacementStrategy
{
    StaticPlacementStrategy(GeometryList const& positions)
        : surface_geometry_by_name(positions)
    {
    }

    msh::SurfaceCreationParameters place(msh::SurfaceCreationParameters const& request_parameters)
    {
        auto placed = request_parameters;
        auto geometry = surface_geometry_by_name[request_parameters.name];

        placed.top_left = geometry.top_left;
        placed.size = geometry.size;
        
        return placed;
    }
    GeometryList surface_geometry_by_name;
};

}

TEST_F(TestClientInput, multiple_clients_receive_motion_inside_windows)
{
    using namespace ::testing;
    
    static int const screen_width = 1000;
    static int const screen_height = 800;
    static int const client_height = screen_width/2;
    static int const client_width = screen_width/2;
    static std::string const test_client_1 = "1";
    static std::string const test_client_2 = "2";
    
    struct ServerConfiguration : mtf::InputTestingServerConfiguration
    {
        std::shared_ptr<msh::PlacementStrategy> the_shell_placement_strategy() override
        {
            static GeometryList positions;
            positions[test_client_1] = geom::Rectangle{geom::Point{geom::X{0}, geom::Y{0}},
                geom::Size{geom::Width{client_width}, geom::Height{client_height}}};
            positions[test_client_2] = geom::Rectangle{geom::Point{geom::X{screen_width/2}, geom::Y{0}},
                geom::Size{geom::Width{client_width}, geom::Height{client_height}}};

            return std::make_shared<StaticPlacementStrategy>(positions);
        }
        
        geom::Rectangle the_screen_geometry() override
        {
            return geom::Rectangle{geom::Point{geom::X{0}, geom::Y{0}},
                    geom::Size{geom::Width{screen_width}, geom::Height{screen_height}}};
        }

        void inject_input() override
        {
            wait_until_client_appears(test_client_1);
            wait_until_client_appears(test_client_2);
            // In the bounds of the first surface
            fake_event_hub->synthesize_event(mis::a_motion_event().with_movement(screen_width/2-1, 0));
            // In the bounds of the second surface
            fake_event_hub->synthesize_event(mis::a_motion_event().with_movement(screen_width/2, 0));
        }
    } server_config;
    
    launch_server_process(server_config);
    
    struct InputClientOne : InputReceivingClient
    {
        InputClientOne()
           : InputReceivingClient(test_client_1)
        {
        }
        
        void expect_input(mt::WaitCondition& events_received) override
        {
            InSequence seq;
            EXPECT_CALL(*handler, handle_input(HoverEnterEvent())).Times(1);
            EXPECT_CALL(*handler, handle_input(MotionEventWithPosition(client_width - 1, 0))).Times(1);
            EXPECT_CALL(*handler, handle_input(HoverExitEvent())).Times(1)
                .WillOnce(mt::WakeUp(&events_received));
        }
    } client_1;

    struct InputClientTwo : InputReceivingClient
    {
        InputClientTwo()
            : InputReceivingClient(test_client_2)
        {
        }
        
        void expect_input(mt::WaitCondition& events_received) override
        {
            InSequence seq;
            EXPECT_CALL(*handler, handle_input(HoverEnterEvent())).Times(1);
            EXPECT_CALL(*handler, handle_input(MotionEventWithPosition(client_width - 1, 0))).Times(1)
                .WillOnce(mt::WakeUp(&events_received));
        }
    } client_2;

    launch_client_process(client_1);
    launch_client_process(client_2);
}
