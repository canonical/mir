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
#include "mir_test/fake_event_hub_input_configuration.h"
#include "mir_test/fake_event_hub.h"
#include "mir_test/event_factory.h"
#include "mir_test/wait_condition.h"
#include "mir_test_framework/display_server_test_fixture.h"

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

#include "mir/input/surface_target.h"

namespace
{
struct FocusNotifyingInputTargeter : public msh::InputTargeter
{
    FocusNotifyingInputTargeter(std::shared_ptr<msh::InputTargeter> const& real_targeter,
                                std::function<void(void)> const& focus_set)
      : real_targeter(real_targeter),
        focus_set(focus_set)
    {

    }
    virtual ~FocusNotifyingInputTargeter() noexcept(true) {}
    
    void focus_changed(std::shared_ptr<mi::SurfaceTarget const> const& surface) override
    {
        real_targeter->focus_changed(surface);

        // We need a synchronization primitive inorder to halt test event injection
        // until after a surface has taken focus (lest the events be discarded).
        focus_set();
    }
    void focus_cleared()
    {
        real_targeter->focus_cleared();
    }

    std::shared_ptr<msh::InputTargeter> const real_targeter;
    std::function<void(void)> focus_set;
};

struct FakeInputServerConfiguration : public mir_test_framework::TestingServerConfiguration
{
    FakeInputServerConfiguration()
      : input_config(the_event_filters(), the_display(), std::shared_ptr<mi::CursorListener>(), the_input_report())
    {
    }

    virtual void inject_input_after_focus()
    {
    }
    
    std::shared_ptr<mi::InputConfiguration> the_input_configuration() override
    {
        fake_event_hub = input_config.the_fake_event_hub();
        fake_event_hub->synthesize_builtin_keyboard_added();
        fake_event_hub->synthesize_builtin_cursor_added();
        fake_event_hub->synthesize_device_scan_complete();

        return mt::fake_shared(input_config);
    }
    
    std::shared_ptr<mi::InputManager> the_input_manager() override
    {
        return DefaultServerConfiguration::the_input_manager();
    }
    std::shared_ptr<ms::InputRegistrar> the_input_registrar() override
    {        
        return DefaultServerConfiguration::the_input_registrar();
    }

    std::shared_ptr<msh::InputTargeter>
    the_input_targeter() override
    {
        return input_targeter(
            [this]()
            {
                return std::make_shared<FocusNotifyingInputTargeter>(DefaultServerConfiguration::the_input_targeter(), std::bind(std::mem_fn(&FakeInputServerConfiguration::inject_input_after_focus), this));
            });
    }

    mtd::FakeEventHubInputConfiguration input_config;
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
    MOCK_METHOD1(handle_input, bool(MirEvent const*));
};

struct InputReceivingClient : ClientConfigCommon
{
    InputReceivingClient(int events_to_receive)
      : events_to_receive(events_to_receive),
        events_received(0)
    {
        assert(events_to_receive <= max_events_to_receive);
    }

    static void handle_input(MirSurface* /* surface */, MirEvent const* ev, void* context)
    {
        auto client = static_cast<InputReceivingClient *>(context);

        if (client->handler->handle_input(ev))
        {
            client->event_received[client->events_received].wake_up_everyone();
            client->events_received++;
        }
    }
    virtual void expect_input()
    {
    }
    
    virtual MirSurfaceParameters parameters()
    {
        MirSurfaceParameters const request_params =
         {
             __PRETTY_FUNCTION__,
             surface_width, surface_height,
             mir_pixel_format_abgr_8888,
             mir_buffer_usage_hardware
         };
        return request_params;
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
    
         MirEventDelegate const event_delegate =
         {
             handle_input,
             this
         };
         auto request_params = parameters();
         mir_wait_for(mir_connection_create_surface(connection, &request_params, create_surface_callback, this));

         mir_surface_set_event_handler(surface, &event_delegate);


         for (int i = 0; i < events_to_receive; i++)
             event_received[i].wait_for_at_most_seconds(5);

         mir_surface_release_sync(surface);
         
         mir_connection_release(connection);

         // ClientConfiguration d'tor is not called on client side so we need this
         // in order to not leak the Mock object.
         handler.reset(); 
    }
    
    std::shared_ptr<MockInputHandler> handler;
    static int const max_events_to_receive = 4;
    mt::WaitCondition event_received[max_events_to_receive];
    
    int events_to_receive;
    int events_received;

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

    struct InputProducingServerConfiguration : FakeInputServerConfiguration
    {
        void inject_input_after_focus()
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
        KeyReceivingClient() : InputReceivingClient(num_events_produced) {}
        void expect_input()
        {
            using namespace ::testing;
            EXPECT_CALL(*handler, handle_input(KeyDownEvent())).Times(num_events_produced)
                .WillRepeatedly(Return(true));
        }
    } client_config;
    launch_client_process(client_config);
}

TEST_F(TestClientInput, clients_receive_us_english_mapped_keys)
{
    using namespace ::testing;
    
    struct InputProducingServerConfiguration : FakeInputServerConfiguration
    {
        void inject_input_after_focus()
        {
            fake_event_hub->synthesize_event(mis::a_key_down_event()
                                             .of_scancode(KEY_LEFTSHIFT));
            fake_event_hub->synthesize_event(mis::a_key_down_event()
                                             .of_scancode(KEY_4));

        }
    } server_config;
    launch_server_process(server_config);
    
    struct KeyReceivingClient : InputReceivingClient
    {
        KeyReceivingClient() : InputReceivingClient(2) {}

        void expect_input()
        {
            using namespace ::testing;

            InSequence seq;
            EXPECT_CALL(*handler, handle_input(AllOf(KeyDownEvent(), KeyOfSymbol(XKB_KEY_Shift_L)))).Times(1)
                .WillOnce(Return(true));
            EXPECT_CALL(*handler, handle_input(AllOf(KeyDownEvent(), KeyOfSymbol(XKB_KEY_dollar)))).Times(1)
                .WillOnce(Return(true));
        }
    } client_config;
    launch_client_process(client_config);
}

// TODO: This assumes that clients are placed by shell at 0,0. Which probably isn't quite safe!
TEST_F(TestClientInput, clients_receive_motion_inside_window)
{
    using namespace ::testing;
    
    struct InputProducingServerConfiguration : FakeInputServerConfiguration
    {
        void inject_input_after_focus()
        {
            fake_event_hub->synthesize_event(mis::a_motion_event().with_movement(InputReceivingClient::surface_width,
                                                                                 InputReceivingClient::surface_height));
            fake_event_hub->synthesize_event(mis::a_motion_event().with_movement(2,2));
        }
    } server_config;
    launch_server_process(server_config);
    
    struct MotionReceivingClient : InputReceivingClient
    {
        MotionReceivingClient() : InputReceivingClient(2) {}

        void expect_input()
        {
            using namespace ::testing;
            
            InSequence seq;

            // We should see the cursor enter
            EXPECT_CALL(*handler, handle_input(HoverEnterEvent())).Times(1).WillOnce(Return(true));
            EXPECT_CALL(*handler, handle_input(
                MotionEventWithPosition(InputReceivingClient::surface_width, 
                                        InputReceivingClient::surface_height))).Times(1).WillOnce(Return(true));
            // But we should not receive an event for the second movement outside of our surface!
        }
    } client_config;
    launch_client_process(client_config);
}

TEST_F(TestClientInput, clients_receive_button_events_inside_window)
{
    using namespace ::testing;
    
    struct InputProducingServerConfiguration : FakeInputServerConfiguration
    {
        void inject_input_after_focus()
        {
            fake_event_hub->synthesize_event(mis::a_button_down_event().of_button(BTN_LEFT).with_action(mis::EventAction::Down));
        }
    } server_config;
    launch_server_process(server_config);
    
    struct ButtonReceivingClient : InputReceivingClient
    {
        ButtonReceivingClient() : InputReceivingClient(1) {}

        void expect_input()
        {
            using namespace ::testing;
            
            InSequence seq;

            // The cursor starts at (0, 0).
            EXPECT_CALL(*handler, handle_input(ButtonDownEvent(0, 0))).Times(1).WillOnce(Return(true));
        }
    } client_config;
    launch_client_process(client_config);
}

namespace
{
struct StubViewableArea : public mg::ViewableArea
{
    StubViewableArea(int width, int height) :
        width(width),
        height(height),
        x(0),
        y(0)
    {
    }
    
    geom::Rectangle view_area() const
    {
        return geom::Rectangle{geom::Point{x, y},
            geom::Size{width, height}};
    }
    
    geom::Width const width;
    geom::Height const height;
    geom::X const x;
    geom::Y const y;
};

typedef std::map<std::string, geom::Point> PositionList;

struct StaticPlacementStrategy : public msh::PlacementStrategy
{
    StaticPlacementStrategy(PositionList positions)
        : surface_positions_by_name(positions)
    {
    }

    msh::SurfaceCreationParameters place(msh::SurfaceCreationParameters const& request_parameters)
    {
        auto placed = request_parameters;
        placed.top_left = surface_positions_by_name[request_parameters.name];
        return placed;
    }
    PositionList surface_positions_by_name;
};

}

TEST_F(TestClientInput, multiple_clients_receive_motion_inside_windows)
{
    using namespace ::testing;
    
    int const screen_width = 1000;
    int const screen_height = 800;
    int const client_width = screen_width/2;
    int const client_height = screen_height;
    std::string const surface1_name = "1";
    std::string const surface2_name = "2";
    
    PositionList positions;
    positions[surface1_name] = geom::Point{geom::X{0}, geom::Y{0}};
    positions[surface2_name] = geom::Point{geom::X{screen_width/2}, geom::Y{0}};

    struct TestServerConfiguration : FakeInputServerConfiguration
    {
        TestServerConfiguration(int screen_width, int screen_height, PositionList positions)
            : screen_width(screen_width),
              screen_height(screen_height),
              positions(positions),
              clients_ready(0)
        {
        }

        std::shared_ptr<mg::ViewableArea> the_viewable_area() override
        {
            return std::make_shared<StubViewableArea>(screen_width, screen_height);
        }
        std::shared_ptr<msh::PlacementStrategy> the_shell_placement_strategy() override
        {
            return std::make_shared<StaticPlacementStrategy>(positions);
        }

        void inject_input_after_focus() override
        {
            clients_ready++;
            if (clients_ready != 2) // We wait until both clients have connected and registered with the input stack.
                return;
            
            // In the bounds of the first surface
            fake_event_hub->synthesize_event(mis::a_motion_event().with_movement(screen_width/2-1, 0));
            // In the bounds of the second surface
            fake_event_hub->synthesize_event(mis::a_motion_event().with_movement(screen_width/2, 0));
        }
        int screen_width, screen_height;
        PositionList positions;

        int clients_ready;
    } server_config(screen_width, screen_height, positions);
    
    launch_server_process(server_config);
    
    MirSurfaceParameters const surface1_params =
        {
            surface1_name.c_str(),
            client_width, client_height,
            mir_pixel_format_abgr_8888,
            mir_buffer_usage_hardware
        };    
    MirSurfaceParameters const surface2_params =
        {
            surface2_name.c_str(),
            client_width, client_height,
            mir_pixel_format_abgr_8888,
            mir_buffer_usage_hardware
        };
    struct ParameterizedClient : InputReceivingClient
    {
        ParameterizedClient(MirSurfaceParameters params, int events_to_expect) : 
            InputReceivingClient(events_to_expect),
            params(params)
        {
        }
        
        MirSurfaceParameters parameters() override
        {
            return params;
        }

        MirSurfaceParameters params;
    };
    struct InputClientOne : ParameterizedClient
    {
        InputClientOne(MirSurfaceParameters params) :
            ParameterizedClient(params, 3)
        {
        }
        
        void expect_input() override
        {
            InSequence seq;
            EXPECT_CALL(*handler, handle_input(HoverEnterEvent())).Times(1).WillOnce(Return(true));
            EXPECT_CALL(*handler, handle_input(
                MotionEventWithPosition(params.width - 1, 0))).Times(1).WillOnce(Return(true));
            EXPECT_CALL(*handler, handle_input(HoverExitEvent())).Times(1).WillOnce(Return(true));
        }
    } client_1(surface1_params);
    struct InputClientTwo : ParameterizedClient
    {
        InputClientTwo(MirSurfaceParameters params) :
            ParameterizedClient(params, 2)
        {
        }
        
        void expect_input() override
        {
            InSequence seq;
            EXPECT_CALL(*handler, handle_input(HoverEnterEvent())).Times(1).WillOnce(Return(true));
            EXPECT_CALL(*handler, handle_input(
                MotionEventWithPosition(params.width - 1, 0))).Times(1).WillOnce(Return(true));
        }
    } client_2(surface2_params);

    launch_client_process(client_1);
    launch_client_process(client_2);
}
