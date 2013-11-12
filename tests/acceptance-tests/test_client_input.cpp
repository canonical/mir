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
#include "mir/shell/surface_creation_parameters.h"
#include "mir/shell/placement_strategy.h"
#include "mir/shell/surface_factory.h"
#include "mir/shell/surface.h"
#include "mir/shell/session_container.h"
#include "mir/shell/session.h"
#include "mir/surfaces/surface_controller.h"
#include "mir/surfaces/surface_stack_model.h"

#include "src/server/input/android/android_input_manager.h"
#include "src/server/input/android/android_input_targeter.h"

#include "mir_toolkit/mir_client_library.h"

#include "mir_test/fake_shared.h"
#include "mir_test/fake_event_hub.h"
#include "mir_test/event_factory.h"
#include "mir_test/wait_condition.h"
#include "mir_test_framework/cross_process_sync.h"
#include "mir_test_framework/display_server_test_fixture.h"
#include "mir_test_framework/input_testing_server_configuration.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <xkbcommon/xkbcommon.h>
#include <xkbcommon/xkbcommon-keysyms.h>

#include <thread>
#include <functional>
#include <map>

#include <poll.h>

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

struct ClientConfig : mtf::TestingClientConfiguration
{
    ClientConfig() :
        connection(0),
        surface(0)
    {
    }

    static void connection_callback(MirConnection* connection, void* context)
    {
        ClientConfig* config = reinterpret_cast<ClientConfig *>(context);
        config->connection = connection;
    }

    static void create_surface_callback(MirSurface* surface, void* context)
    {
        ClientConfig* config = reinterpret_cast<ClientConfig *>(context);
        config->surface_created(surface);
    }

    static void release_surface_callback(MirSurface* surface, void* context)
    {
        ClientConfig* config = reinterpret_cast<ClientConfig *>(context);
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

struct InputClient : ClientConfig
{
    InputClient(const mtf::CrossProcessSync& input_cb_setup_fence, std::string const& client_name)
            : input_cb_setup_fence(input_cb_setup_fence),
              client_name(client_name)
    {
    }

    virtual void surface_created(MirSurface* new_surface) override
    {
        ClientConfig::surface_created(new_surface);

        MirEventDelegate const event_delegate =
        {
            handle_input,
            this
        };

        // Set this in the callback, not main thread to avoid missing test events
        mir_surface_set_event_handler(surface, &event_delegate);

        try
        {
            input_cb_setup_fence.try_signal_ready_for();
        } catch (const std::runtime_error& e)
        {
            std::cout << e.what() << std::endl;
        }

    }

    static void handle_input(MirSurface* /* surface */, MirEvent const* ev, void* context)
    {
        if (ev->type == mir_event_type_surface)
            return;

        auto client = static_cast<InputClient *>(context);
        client->handler->handle_input(ev);
    }

    virtual void expect_input(mt::WaitCondition&) = 0;

    virtual MirSurfaceParameters parameters()
    {
        MirSurfaceParameters const request_params =
         {
             client_name.c_str(),
             surface_width, surface_height,
             mir_pixel_format_abgr_8888,
             mir_buffer_usage_hardware,
             mir_display_output_id_invalid
         };
        return request_params;
    }

    void exec()
    {
        handler = std::make_shared<MockInputHandler>();

        expect_input(events_received);

        mir_wait_for(mir_connect(
            mir_test_socket,
            client_name.c_str(),
            connection_callback,
            this));
         ASSERT_TRUE(connection != NULL);

         auto request_params = parameters();
         mir_wait_for(mir_connection_create_surface(connection, &request_params, create_surface_callback, this));

         events_received.wait_for_at_most_seconds(2);

         mir_surface_release_sync(surface);

         mir_connection_release(connection);

         // ClientConfiguration d'tor is not called on client side so we need this
         // in order to not leak the Mock object.
         handler.reset();
    }

    std::shared_ptr<MockInputHandler> handler;
    mt::WaitCondition events_received;

    mtf::CrossProcessSync input_cb_setup_fence;
    std::string const client_name;

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

MATCHER_P2(ButtonUpEvent, x, y, "")
{
    if (arg->type != mir_event_type_motion)
        return false;
    if (arg->motion.action != mir_motion_action_up)
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

MATCHER(MovementEvent, "")
{
    if (arg->type != mir_event_type_motion)
        return false;
    if (arg->motion.action != mir_motion_action_move &&
        arg->motion.action != mir_motion_action_hover_move)
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

    mtf::CrossProcessSync fence;

    struct ServerConfiguration : mtf::InputTestingServerConfiguration
    {
        mtf::CrossProcessSync input_cb_setup_fence;

        ServerConfiguration(const mtf::CrossProcessSync& input_cb_setup_fence) 
                : input_cb_setup_fence(input_cb_setup_fence)
        {
        }
        
        void inject_input()
        {
            wait_until_client_appears(test_client_name);
            input_cb_setup_fence.wait_for_signal_ready_for();

            for (int i = 0; i < num_events_produced; i++)
                fake_event_hub->synthesize_event(mis::a_key_down_event()
                                                 .of_scancode(KEY_ENTER));
        }
    } server_config(fence);
    launch_server_process(server_config);
    
    struct KeyReceivingClient : InputClient
    {
        KeyReceivingClient(const mtf::CrossProcessSync& fence) : InputClient(fence, test_client_name) {}
        void expect_input(mt::WaitCondition& events_received) override
        {
            using namespace ::testing;
            InSequence seq;

            EXPECT_CALL(*handler, handle_input(KeyDownEvent())).Times(2);
            EXPECT_CALL(*handler, handle_input(KeyDownEvent())).Times(1)
                .WillOnce(mt::WakeUp(&events_received));
        }
    } client_config(fence);
    launch_client_process(client_config);
}

TEST_F(TestClientInput, clients_receive_us_english_mapped_keys)
{
    using namespace ::testing;
    static std::string const test_client_name = "1";
    mtf::CrossProcessSync fence;

    struct ServerConfiguration : mtf::InputTestingServerConfiguration
    {
        mtf::CrossProcessSync input_cb_setup_fence;

        ServerConfiguration(const mtf::CrossProcessSync& input_cb_setup_fence) 
                : input_cb_setup_fence(input_cb_setup_fence)
        {
        }

        void inject_input()
        {
            wait_until_client_appears(test_client_name);
            input_cb_setup_fence.wait_for_signal_ready_for();

            fake_event_hub->synthesize_event(mis::a_key_down_event()
                                             .of_scancode(KEY_LEFTSHIFT));
            fake_event_hub->synthesize_event(mis::a_key_down_event()
                                             .of_scancode(KEY_4));

        }
    } server_config{fence};
    launch_server_process(server_config);
    
    struct KeyReceivingClient : InputClient
    {
        KeyReceivingClient(const mtf::CrossProcessSync& fence) : InputClient(fence, test_client_name) {}

        void expect_input(mt::WaitCondition& events_received) override
        {
            using namespace ::testing;

            InSequence seq;
            EXPECT_CALL(*handler, handle_input(AllOf(KeyDownEvent(), KeyOfSymbol(XKB_KEY_Shift_L)))).Times(1);
            EXPECT_CALL(*handler, handle_input(AllOf(KeyDownEvent(), KeyOfSymbol(XKB_KEY_dollar)))).Times(1)
                .WillOnce(mt::WakeUp(&events_received));
        }
    } client_config{fence};
    launch_client_process(client_config);
}

TEST_F(TestClientInput, clients_receive_motion_inside_window)
{
    using namespace ::testing;
    static std::string const test_client_name = "1";
    mtf::CrossProcessSync fence;

    struct ServerConfiguration : public mtf::InputTestingServerConfiguration
    {
        mtf::CrossProcessSync input_cb_setup_fence;

        ServerConfiguration(const mtf::CrossProcessSync& input_cb_setup_fence) 
                : input_cb_setup_fence(input_cb_setup_fence)
        {
        }

        void inject_input()
        {
            wait_until_client_appears(test_client_name);
            input_cb_setup_fence.wait_for_signal_ready_for();

            fake_event_hub->synthesize_event(mis::a_motion_event().with_movement(InputClient::surface_width - 1,
                                                                                 InputClient::surface_height - 1));
            fake_event_hub->synthesize_event(mis::a_motion_event().with_movement(2,2));
        }
    } server_config{fence};
    launch_server_process(server_config);
    
    struct MotionReceivingClient : InputClient
    {
        MotionReceivingClient(const mtf::CrossProcessSync& fence) : InputClient(fence, test_client_name) {}

        void expect_input(mt::WaitCondition& events_received) override
        {
            using namespace ::testing;
            
            InSequence seq;

            // We should see the cursor enter
            EXPECT_CALL(*handler, handle_input(HoverEnterEvent())).Times(1);
            EXPECT_CALL(*handler, handle_input(
                MotionEventWithPosition(InputClient::surface_width - 1,
                                        InputClient::surface_height - 1))).Times(1)
                .WillOnce(mt::WakeUp(&events_received));
            // But we should not receive an event for the second movement outside of our surface!
        }
    } client_config{fence};
    launch_client_process(client_config);
}

TEST_F(TestClientInput, clients_receive_button_events_inside_window)
{
    using namespace ::testing;
    
    static std::string const test_client_name = "1";
    mtf::CrossProcessSync fence;

    struct ServerConfiguration : public mtf::InputTestingServerConfiguration
    {
        mtf::CrossProcessSync input_cb_setup_fence;

        ServerConfiguration(const mtf::CrossProcessSync& input_cb_setup_fence) 
                : input_cb_setup_fence(input_cb_setup_fence)
        {
        }

        void inject_input()
        {
            wait_until_client_appears(test_client_name);
            input_cb_setup_fence.wait_for_signal_ready_for();

            fake_event_hub->synthesize_event(mis::a_button_down_event().of_button(BTN_LEFT).with_action(mis::EventAction::Down));
        }
    } server_config{fence};
    launch_server_process(server_config);
    
    struct ButtonReceivingClient : InputClient
    {
        ButtonReceivingClient(const mtf::CrossProcessSync& fence) : InputClient(fence, test_client_name) {}

        void expect_input(mt::WaitCondition& events_received) override
        {
            using namespace ::testing;
            
            InSequence seq;

            // The cursor starts at (0, 0).
            EXPECT_CALL(*handler, handle_input(ButtonDownEvent(0, 0))).Times(1)
                .WillOnce(mt::WakeUp(&events_received));
        }
    } client_config{fence};
    launch_client_process(client_config);
}

namespace
{
typedef std::map<std::string, geom::Rectangle> GeometryMap;
typedef std::map<std::string, ms::DepthId> DepthMap;

struct StaticPlacementStrategy : public msh::PlacementStrategy
{
    StaticPlacementStrategy(GeometryMap const& positions,
                            DepthMap const& depths)
        : surface_geometry_by_name(positions),
          surface_depths_by_name(depths)
    {
    }

    StaticPlacementStrategy(GeometryMap const& positions)
        : StaticPlacementStrategy(positions, DepthMap())
    {
    }

    msh::SurfaceCreationParameters place(msh::Session const&, msh::SurfaceCreationParameters const& request_parameters)
    {
        auto placed = request_parameters;
        auto const& name = request_parameters.name;
        auto geometry = surface_geometry_by_name[name];

        placed.top_left = geometry.top_left;
        placed.size = geometry.size;
        placed.depth = surface_depths_by_name[name];
        
        return placed;
    }
    GeometryMap surface_geometry_by_name;
    DepthMap surface_depths_by_name;
};

}

TEST_F(TestClientInput, multiple_clients_receive_motion_inside_windows)
{
    using namespace ::testing;
    
    static int const screen_width = 1000;
    static int const screen_height = 800;
    static int const client_height = screen_height/2;
    static int const client_width = screen_width/2;
    static std::string const test_client_1 = "1";
    static std::string const test_client_2 = "2";
    mtf::CrossProcessSync fence;

    struct ServerConfiguration : mtf::InputTestingServerConfiguration
    {
        mtf::CrossProcessSync input_cb_setup_fence;

        ServerConfiguration(const mtf::CrossProcessSync& input_cb_setup_fence) 
                : input_cb_setup_fence(input_cb_setup_fence)
        {
        }

        std::shared_ptr<msh::PlacementStrategy> the_shell_placement_strategy() override
        {
            static GeometryMap positions;
            positions[test_client_1] = geom::Rectangle{geom::Point{0, 0},
                geom::Size{client_width, client_height}};
            positions[test_client_2] = geom::Rectangle{geom::Point{screen_width/2, screen_height/2},
                geom::Size{client_width, client_height}};

            return std::make_shared<StaticPlacementStrategy>(positions);
        }
        
        void inject_input() override
        {
            wait_until_client_appears(test_client_1);
            EXPECT_EQ(1, input_cb_setup_fence.wait_for_signal_ready_for());
            wait_until_client_appears(test_client_2);
            EXPECT_EQ(2, input_cb_setup_fence.wait_for_signal_ready_for());

            // In the bounds of the first surface
            fake_event_hub->synthesize_event(mis::a_motion_event().with_movement(screen_width/2-1, screen_height/2-1));
            // In the bounds of the second surface
            fake_event_hub->synthesize_event(mis::a_motion_event().with_movement(screen_width/2, screen_height/2));
        }
    } server_config{fence};
    
    launch_server_process(server_config);
    
    struct InputClientOne : InputClient
    {
        InputClientOne(const mtf::CrossProcessSync& fence)
                : InputClient(fence, test_client_1)
        {
        }
        
        void expect_input(mt::WaitCondition& events_received) override
        {
            InSequence seq;
            EXPECT_CALL(*handler, handle_input(HoverEnterEvent())).Times(1);
            EXPECT_CALL(*handler, handle_input(MotionEventWithPosition(client_width - 1, client_height - 1))).Times(1);
            EXPECT_CALL(*handler, handle_input(HoverExitEvent())).Times(1)
                .WillOnce(mt::WakeUp(&events_received));
        }
    } client_1{fence};

    struct InputClientTwo : InputClient
    {
        InputClientTwo(const mtf::CrossProcessSync& fence)
                : InputClient(fence, test_client_2)
        {
        }
        
        void expect_input(mt::WaitCondition& events_received) override
        {
            InSequence seq;
            EXPECT_CALL(*handler, handle_input(HoverEnterEvent())).Times(1);
            EXPECT_CALL(*handler, handle_input(MotionEventWithPosition(client_width - 1, client_height - 1))).Times(1)
                .WillOnce(mt::WakeUp(&events_received));
        }
    } client_2{fence};

    launch_client_process(client_1);
    launch_client_process(client_2);
}

namespace
{
struct RegionApplyingSurfaceFactory : public msh::SurfaceFactory
{
    RegionApplyingSurfaceFactory(std::shared_ptr<msh::SurfaceFactory> real_factory,
        std::initializer_list<geom::Rectangle> const& input_rectangles)
        : underlying_factory(real_factory),
          input_rectangles(input_rectangles)
    {
    }
    
    std::shared_ptr<msh::Surface> create_surface(msh::Session* session,
                                                 msh::SurfaceCreationParameters const& params,
                                                 mf::SurfaceId id,
                                                 std::shared_ptr<mf::EventSink> const& sink)
    {
        auto surface = underlying_factory->create_surface(session, params, id, sink);

        surface->set_input_region(input_rectangles);

        return surface;
    }
    
    std::shared_ptr<msh::SurfaceFactory> const underlying_factory;
    std::vector<geom::Rectangle> const input_rectangles;
};
}
TEST_F(TestClientInput, clients_do_not_receive_motion_outside_input_region)
{
    using namespace ::testing;
    static std::string const test_client_name = "1";
    mtf::CrossProcessSync fence;

    static int const screen_width = 100;
    static int const screen_height = 100;
    
    static geom::Rectangle const screen_geometry{geom::Point{0, 0},
        geom::Size{screen_width, screen_height}};

    static std::initializer_list<geom::Rectangle> client_input_regions = {
        {geom::Point{0, 0}, {screen_width-80, screen_height}},
        {geom::Point{screen_width-20, 0}, {screen_width-80, screen_height}}
    };

    struct ServerConfiguration : mtf::InputTestingServerConfiguration
    {
        mtf::CrossProcessSync input_cb_setup_fence;

        ServerConfiguration(const mtf::CrossProcessSync& input_cb_setup_fence) 
                : input_cb_setup_fence(input_cb_setup_fence)
        {
        }

        std::shared_ptr<msh::PlacementStrategy> the_shell_placement_strategy() override
        {
            static GeometryMap positions;
            positions[test_client_name] = screen_geometry;
            
            return std::make_shared<StaticPlacementStrategy>(positions);
        }
        std::shared_ptr<msh::SurfaceFactory> the_shell_surface_factory() override
        {
            return std::make_shared<RegionApplyingSurfaceFactory>(InputTestingServerConfiguration::the_shell_surface_factory(),
                client_input_regions);
        }
        
        void inject_input() override
        {
            wait_until_client_appears(test_client_name);
            input_cb_setup_fence.wait_for_signal_ready_for();
            
            // First we will move the cursor in to the input region on the left side of the window. We should see a click here
            fake_event_hub->synthesize_event(mis::a_motion_event().with_movement(1, 1));
            fake_event_hub->synthesize_event(mis::a_button_down_event().of_button(BTN_LEFT).with_action(mis::EventAction::Down));
            fake_event_hub->synthesize_event(mis::a_button_up_event().of_button(BTN_LEFT));
            // Now in to the dead zone in the center of the window. We should not see a click here.
            fake_event_hub->synthesize_event(mis::a_motion_event().with_movement(49, 49));
            fake_event_hub->synthesize_event(mis::a_button_down_event().of_button(BTN_LEFT).with_action(mis::EventAction::Down));
            fake_event_hub->synthesize_event(mis::a_button_up_event().of_button(BTN_LEFT));
            // Now in to the right edge of the window, in the right input region. Again we should see a click
            fake_event_hub->synthesize_event(mis::a_motion_event().with_movement(49, 49));
            fake_event_hub->synthesize_event(mis::a_button_down_event().of_button(BTN_LEFT).with_action(mis::EventAction::Down));
            fake_event_hub->synthesize_event(mis::a_button_up_event().of_button(BTN_LEFT));
        }
    } server_config{fence};
    
    launch_server_process(server_config);

    struct ClientConfig : InputClient
    {
        ClientConfig(const mtf::CrossProcessSync& fence)
                : InputClient(fence, test_client_name)
        {
        }

        void expect_input(mt::WaitCondition& events_received) override
        {

            EXPECT_CALL(*handler, handle_input(HoverEnterEvent())).Times(AnyNumber());
            EXPECT_CALL(*handler, handle_input(HoverExitEvent())).Times(AnyNumber());
            EXPECT_CALL(*handler, handle_input(MovementEvent())).Times(AnyNumber());

            {
                // We should see two of the three button pairs.
                InSequence seq;
                EXPECT_CALL(*handler, handle_input(ButtonDownEvent(1, 1))).Times(1);
                EXPECT_CALL(*handler, handle_input(ButtonUpEvent(1, 1))).Times(1);
                EXPECT_CALL(*handler, handle_input(ButtonDownEvent(99, 99))).Times(1);
                EXPECT_CALL(*handler, handle_input(ButtonUpEvent(99, 99))).Times(1)
                    .WillOnce(mt::WakeUp(&events_received));
            }
        }
    } client_config{fence};
    launch_client_process(client_config);
}

TEST_F(TestClientInput, surfaces_obscure_motion_events_by_stacking)
{
    using namespace ::testing;
    
    static std::string const test_client_name_1 = "1";
    static std::string const test_client_name_2 = "2";
    mtf::CrossProcessSync fence;

    static int const screen_width = 100;
    static int const screen_height = 100;
    
    static geom::Rectangle const screen_geometry{geom::Point{0, 0},
        geom::Size{screen_width, screen_height}};

    struct ServerConfiguration : mtf::InputTestingServerConfiguration
    {
        mtf::CrossProcessSync input_cb_setup_fence;

        ServerConfiguration(const mtf::CrossProcessSync& input_cb_setup_fence) 
                : input_cb_setup_fence(input_cb_setup_fence)
        {
        }

        std::shared_ptr<msh::PlacementStrategy> the_shell_placement_strategy() override
        {
            static GeometryMap positions;
            positions[test_client_name_1] = screen_geometry;
            
            auto smaller_geometry = screen_geometry;
            smaller_geometry.size.width = geom::Width{screen_width/2};
            positions[test_client_name_2] = smaller_geometry;
            
            static DepthMap depths;
            depths[test_client_name_1] = ms::DepthId{0};
            depths[test_client_name_2] = ms::DepthId{1};
            
            return std::make_shared<StaticPlacementStrategy>(positions, depths);
        }
        
        void inject_input() override
        {
            wait_until_client_appears(test_client_name_1);
            input_cb_setup_fence.wait_for_signal_ready_for();
            wait_until_client_appears(test_client_name_2);
            input_cb_setup_fence.wait_for_signal_ready_for();

            // First we will move the cursor in to the region where client 2 obscures client 1
            fake_event_hub->synthesize_event(mis::a_motion_event().with_movement(1, 1));
            fake_event_hub->synthesize_event(mis::a_button_down_event().of_button(BTN_LEFT).with_action(mis::EventAction::Down));
            fake_event_hub->synthesize_event(mis::a_button_up_event().of_button(BTN_LEFT));
            // Now we move to the unobscured region of client 1
            fake_event_hub->synthesize_event(mis::a_motion_event().with_movement(50, 0));
            fake_event_hub->synthesize_event(mis::a_button_down_event().of_button(BTN_LEFT).with_action(mis::EventAction::Down));
            fake_event_hub->synthesize_event(mis::a_button_up_event().of_button(BTN_LEFT));
        }
    } server_config{fence};
    
    launch_server_process(server_config);

    struct ClientConfigOne : InputClient
    {
        ClientConfigOne(const mtf::CrossProcessSync& fence)
                : InputClient(fence, test_client_name_1)
        {
        }

        void expect_input(mt::WaitCondition& events_received) override
        {
            EXPECT_CALL(*handler, handle_input(HoverEnterEvent())).Times(AnyNumber());
            EXPECT_CALL(*handler, handle_input(HoverExitEvent())).Times(AnyNumber());
            EXPECT_CALL(*handler, handle_input(MovementEvent())).Times(AnyNumber());

            {
                // We should only see one button event sequence.
                InSequence seq;
                EXPECT_CALL(*handler, handle_input(ButtonDownEvent(51, 1))).Times(1);
                EXPECT_CALL(*handler, handle_input(ButtonUpEvent(51, 1))).Times(1)
                    .WillOnce(mt::WakeUp(&events_received));
            }
        }
    } client_config_1{fence};
    launch_client_process(client_config_1);

    struct ClientConfigTwo : InputClient
    {
        ClientConfigTwo(const mtf::CrossProcessSync& fence)
                : InputClient(fence, test_client_name_2)
        {
        }

        void expect_input(mt::WaitCondition& events_received) override
        {
            EXPECT_CALL(*handler, handle_input(HoverEnterEvent())).Times(AnyNumber());
            EXPECT_CALL(*handler, handle_input(HoverExitEvent())).Times(AnyNumber());
            EXPECT_CALL(*handler, handle_input(MovementEvent())).Times(AnyNumber());

            {
                // Likewise we should only see one button sequence.
              InSequence seq;
              EXPECT_CALL(*handler, handle_input(ButtonDownEvent(1, 1))).Times(1);
              EXPECT_CALL(*handler, handle_input(ButtonUpEvent(1, 1))).Times(1)
                  .WillOnce(mt::WakeUp(&events_received));
            }
        }
    } client_config_2{fence};
    launch_client_process(client_config_2);
}

namespace
{

ACTION_P(SignalFence, fence)
{
    fence->try_signal_ready_for();
}

}

TEST_F(TestClientInput, hidden_clients_do_not_receive_pointer_events)
{
    using namespace ::testing;
    
    static std::string const test_client_name = "1";
    static std::string const test_client_2_name = "2";
    mtf::CrossProcessSync fence, first_client_ready_fence, second_client_done_fence;

    struct ServerConfiguration : public mtf::InputTestingServerConfiguration
    {
        mtf::CrossProcessSync input_cb_setup_fence;
        mtf::CrossProcessSync second_client_done_fence;

        ServerConfiguration(const mtf::CrossProcessSync& input_cb_setup_fence,
                            const mtf::CrossProcessSync& second_client_done_fence) 
                : input_cb_setup_fence(input_cb_setup_fence),
                  second_client_done_fence(second_client_done_fence)
        {
        }
        
        void hide_session_by_name(std::string const& session_name)
        {
            the_shell_session_container()->for_each([&](std::shared_ptr<msh::Session> const& session) -> void
            {
                if (session->name() == session_name)
                    session->hide();
            });
        }

        void inject_input()
        {
            wait_until_client_appears(test_client_name);
            wait_until_client_appears(test_client_2_name);
            input_cb_setup_fence.wait_for_signal_ready_for();

            // We send one event and then hide the surface on top before sending the next. 
            // So we expect each of the two surfaces to receive one event pair.
            fake_event_hub->synthesize_event(mis::a_motion_event().with_movement(1,1));

            // We use a fence to ensure we do not hide the client
            // before event dispatch occurs
            second_client_done_fence.wait_for_signal_ready_for();
            hide_session_by_name(test_client_2_name);

            fake_event_hub->synthesize_event(mis::a_motion_event().with_movement(1,1));
        }
    } server_config{fence, second_client_done_fence};
    launch_server_process(server_config);
    
    struct ButtonClientOne : InputClient
    {
        ButtonClientOne(const mtf::CrossProcessSync& fence)
                : InputClient(fence, test_client_name)
        {
        }
        
        void expect_input(mt::WaitCondition& events_received) override
        {
            EXPECT_CALL(*handler, handle_input(HoverEnterEvent())).Times(AnyNumber());
            EXPECT_CALL(*handler, handle_input(HoverExitEvent())).Times(AnyNumber());
            EXPECT_CALL(*handler, handle_input(MotionEventWithPosition(2, 2))).Times(1)
                .WillOnce(mt::WakeUp(&events_received));
        }
    } client_1{first_client_ready_fence};
    struct ButtonClientTwo : InputClient
    {
        mtf::CrossProcessSync first_client_ready;
        mtf::CrossProcessSync done_fence;

        ButtonClientTwo(mtf::CrossProcessSync const& fence, mtf::CrossProcessSync const& first_client_ready,
                        mtf::CrossProcessSync const& done_fence) 
            : InputClient(fence, test_client_2_name),
              first_client_ready(first_client_ready),
              done_fence(done_fence)
        {
        }
        void exec()
        {
            // Ensure we stack on top of the first client
            first_client_ready.wait_for_signal_ready_for();
            InputClient::exec();
        }

        void expect_input(mt::WaitCondition& events_received) override
        {
            EXPECT_CALL(*handler, handle_input(HoverEnterEvent())).Times(AnyNumber());
            EXPECT_CALL(*handler, handle_input(HoverExitEvent())).Times(AnyNumber());
            EXPECT_CALL(*handler, handle_input(MotionEventWithPosition(1, 1))).Times(1)
                .WillOnce(DoAll(SignalFence(&done_fence), mt::WakeUp(&events_received)));
        }
    } client_2{fence, first_client_ready_fence, second_client_done_fence};

    // Client 2 is launched second so will be the first to receive input

    launch_client_process(client_1);
    launch_client_process(client_2);
}
