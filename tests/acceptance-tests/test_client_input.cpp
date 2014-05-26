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

#include "mir/scene/surface_creation_parameters.h"
#include "mir/scene/placement_strategy.h"
#include "mir/scene/surface_coordinator.h"
#include "mir/scene/surface.h"
#include "src/server/scene/session_container.h"
#include "mir/scene/session.h"

#include "mir_test/fake_event_hub.h"
#include "mir_test/wait_condition.h"
#include "mir_test/client_event_matchers.h"
#include "mir_test_framework/cross_process_sync.h"
#include "mir_test_framework/display_server_test_fixture.h"
#include "mir_test_framework/input_testing_server_configuration.h"
#include "mir_test_framework/input_testing_client_configuration.h"
#include "mir_test_framework/declarative_placement_strategy.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mi = mir::input;
namespace mis = mi::synthesis;
namespace msh = mir::shell;
namespace ms = mir::scene;
namespace geom = mir::geometry;
namespace mt = mir::test;
namespace mtd = mt::doubles;
namespace mtf = mir_test_framework;

namespace
{
struct ServerConfiguration : mtf::InputTestingServerConfiguration
{
    mtf::CrossProcessSync input_cb_setup_fence;
    int number_of_clients = 1;
    std::function<void(mtf::InputTestingServerConfiguration& server)> produce_events;
    mtf::SurfaceGeometries client_geometries;
    mtf::SurfaceDepths client_depths;

    ServerConfiguration(mtf::CrossProcessSync const& input_cb_setup_fence)
            : input_cb_setup_fence(input_cb_setup_fence)
    {
    }

    std::shared_ptr<ms::PlacementStrategy> the_placement_strategy() override
    {
        return std::make_shared<mtf::DeclarativePlacementStrategy>(
            InputTestingServerConfiguration::the_placement_strategy(),
            client_geometries, client_depths);
    }

    void inject_input()
    {
        for (int i = 1; i < number_of_clients + 1; i++)
            EXPECT_EQ(i, input_cb_setup_fence.wait_for_signal_ready_for());
        produce_events(*this);
    }
};

struct ClientConfig : mtf::InputTestingClientConfiguration
{
    std::function<void(MockInputHandler&, mt::WaitCondition&)> expect_cb;

    ClientConfig(std::string const& client_name, mtf::CrossProcessSync const& client_ready_fence)
        : InputTestingClientConfiguration(client_name, client_ready_fence)
    {
    }

    void expect_input(MockInputHandler &handler, mt::WaitCondition& events_received) override
    {
        expect_cb(handler, events_received);
    }
};

struct TestClientInput : BespokeDisplayServerTestFixture
{
    mtf::CrossProcessSync fence;
    ServerConfiguration server_config{fence};

    std::string const arbitrary_client_name{"input-test-client"};
    std::string const test_client_name_1 = "1";
    std::string const test_client_name_2 = "2";

    ClientConfig client_config{arbitrary_client_name, fence};
    ClientConfig client_config_1{test_client_name_1, fence};
    ClientConfig client_config_2{test_client_name_2, fence};
};
}

using namespace ::testing;
using MockHandler = mtf::InputTestingClientConfiguration::MockInputHandler;


TEST_F(TestClientInput, clients_receive_key_input)
{
    server_config.produce_events = [&](mtf::InputTestingServerConfiguration& server)
         {
             int const num_events_produced = 3;

             for (int i = 0; i < num_events_produced; i++)
                 server.fake_event_hub->synthesize_event(mis::a_key_down_event()
                                                         .of_scancode(KEY_ENTER));
         };

    launch_server_process(server_config);

    client_config.expect_cb = [&](MockHandler& handler, mt::WaitCondition& events_received)
        {
            InSequence seq;

            EXPECT_CALL(handler, handle_input(mt::KeyDownEvent())).Times(2);
            EXPECT_CALL(handler, handle_input(mt::KeyDownEvent())).Times(1)
                .WillOnce(mt::WakeUp(&events_received));

        };
    launch_client_process(client_config);
}

TEST_F(TestClientInput, clients_receive_us_english_mapped_keys)
{
    server_config.produce_events = [&](mtf::InputTestingServerConfiguration& server)
        {
            server.fake_event_hub->synthesize_event(mis::a_key_down_event()
                .of_scancode(KEY_LEFTSHIFT));
            server.fake_event_hub->synthesize_event(mis::a_key_down_event()
                .of_scancode(KEY_4));
        };
    launch_server_process(server_config);

    client_config.expect_cb =  [&](MockHandler& handler, mt::WaitCondition& events_received)
        {
            InSequence seq;

            EXPECT_CALL(handler, handle_input(AllOf(mt::KeyDownEvent(), mt::KeyOfSymbol(XKB_KEY_Shift_L)))).Times(1);
            EXPECT_CALL(handler, handle_input(AllOf(mt::KeyDownEvent(), mt::KeyOfSymbol(XKB_KEY_dollar)))).Times(1)
                .WillOnce(mt::WakeUp(&events_received));
        };
    launch_client_process(client_config);
}

TEST_F(TestClientInput, clients_receive_motion_inside_window)
{
    server_config.produce_events = [&](mtf::InputTestingServerConfiguration& server)
        {
             server.fake_event_hub->synthesize_event(mis::a_motion_event().with_movement(mtf::InputTestingClientConfiguration::surface_width - 1,
                 mtf::InputTestingClientConfiguration::surface_height - 1));
             server.fake_event_hub->synthesize_event(mis::a_motion_event().with_movement(2,2));
        };
    launch_server_process(server_config);

    client_config.expect_cb = [&](MockHandler& handler, mt::WaitCondition& events_received)
        {
            InSequence seq;

            // We should see the cursor enter
            EXPECT_CALL(handler, handle_input(mt::HoverEnterEvent())).Times(1);
            EXPECT_CALL(handler, handle_input(
                mt::MotionEventWithPosition(mtf::InputTestingClientConfiguration::surface_width - 1,
                                        mtf::InputTestingClientConfiguration::surface_height - 1))).Times(1)
                .WillOnce(mt::WakeUp(&events_received));
            // But we should not receive an event for the second movement outside of our surface!
        };
    launch_client_process(client_config);
}

TEST_F(TestClientInput, clients_receive_button_events_inside_window)
{
    server_config.produce_events = [&](mtf::InputTestingServerConfiguration& server)
        {
             server.fake_event_hub->synthesize_event(mis::a_button_down_event()
                 .of_button(BTN_LEFT).with_action(mis::EventAction::Down));
        };
    launch_server_process(server_config);

    client_config.expect_cb = [&](MockHandler& handler, mt::WaitCondition& events_received)
        {
            InSequence seq;

            // The cursor starts at (0, 0).
            EXPECT_CALL(handler, handle_input(mt::ButtonDownEvent(0, 0))).Times(1)
                .WillOnce(mt::WakeUp(&events_received));
        };
    launch_client_process(client_config);
}

TEST_F(TestClientInput, multiple_clients_receive_motion_inside_windows)
{
    static int const screen_width = 1000;
    static int const screen_height = 800;
    static int const client_height = screen_height/2;
    static int const client_width = screen_width/2;

    server_config.number_of_clients = 2;
    server_config.produce_events = [&](mtf::InputTestingServerConfiguration& server)
        {
            // In the bounds of the first surface
            server.fake_event_hub->synthesize_event(mis::a_motion_event().with_movement(screen_width/2-1, screen_height/2-1));
            // In the bounds of the second surface
            server.fake_event_hub->synthesize_event(mis::a_motion_event().with_movement(screen_width/2, screen_height/2));
        };
    server_config.client_geometries[test_client_name_1] = {{0, 0}, {client_width, client_height}};
    server_config.client_geometries[test_client_name_2] = {{screen_width/2, screen_height/2}, {client_width, client_height}};
    launch_server_process(server_config);

    client_config_1.expect_cb = [&](MockHandler& handler, mt::WaitCondition& events_received)
        {
            InSequence seq;
            EXPECT_CALL(handler, handle_input(mt::HoverEnterEvent())).Times(1);
            EXPECT_CALL(handler, handle_input(mt::MotionEventWithPosition(client_width - 1, client_height - 1))).Times(1);
            EXPECT_CALL(handler, handle_input(mt::HoverExitEvent())).Times(1)
                .WillOnce(mt::WakeUp(&events_received));
        };
    client_config_2.expect_cb = [&](MockHandler& handler, mt::WaitCondition& events_received)
        {
             InSequence seq;
             EXPECT_CALL(handler, handle_input(mt::HoverEnterEvent())).Times(1);
             EXPECT_CALL(handler, handle_input(mt::MotionEventWithPosition(client_width - 1, client_height - 1))).Times(1)
                 .WillOnce(mt::WakeUp(&events_received));
        };

    launch_client_process(client_config_1);
    launch_client_process(client_config_2);
}

namespace
{
struct RegionApplyingSurfaceCoordinator : public ms::SurfaceCoordinator
{
    RegionApplyingSurfaceCoordinator(std::shared_ptr<ms::SurfaceCoordinator> wrapped_coordinator,
        std::initializer_list<geom::Rectangle> const& input_rectangles)
        : wrapped_coordinator(wrapped_coordinator),
          input_rectangles(input_rectangles)
    {
    }

    std::shared_ptr<ms::Surface> add_surface(
        ms::SurfaceCreationParameters const& params,
        ms::Session* session) override
    {
        auto surface = wrapped_coordinator->add_surface(params, session);

        surface->set_input_region(input_rectangles);

        return surface;
    }

    void remove_surface(std::weak_ptr<ms::Surface> const& surface) override
    {
        wrapped_coordinator->remove_surface(surface);
    }

    void raise(std::weak_ptr<ms::Surface> const& surface) override
    {
        wrapped_coordinator->raise(surface);
    }

    std::shared_ptr<ms::SurfaceCoordinator> const wrapped_coordinator;
    std::vector<geom::Rectangle> const input_rectangles;
};
}

TEST_F(TestClientInput, clients_do_not_receive_motion_outside_input_region)
{
    static int const screen_width = 100;
    static int const screen_height = 100;

    static std::initializer_list<geom::Rectangle> client_input_regions = {
        {geom::Point{0, 0}, {screen_width-80, screen_height}},
        {geom::Point{screen_width-20, 0}, {screen_width-80, screen_height}}
    };

    struct LocalServerConfiguration : ::ServerConfiguration
    {
        using ServerConfiguration::ServerConfiguration;

        std::shared_ptr<ms::SurfaceCoordinator> the_surface_coordinator() override
        {
            return std::make_shared<RegionApplyingSurfaceCoordinator>(ServerConfiguration::the_surface_coordinator(), client_input_regions);
        }
    } server_config{fence};

    server_config.produce_events = [&](mtf::InputTestingServerConfiguration& server)
        {
            // First we will move the cursor in to the input region on the left side of the window. We should see a click here
            server.fake_event_hub->synthesize_event(mis::a_motion_event().with_movement(1, 1));
            server.fake_event_hub->synthesize_event(mis::a_button_down_event().of_button(BTN_LEFT).with_action(mis::EventAction::Down));
            server.fake_event_hub->synthesize_event(mis::a_button_up_event().of_button(BTN_LEFT));
            // Now in to the dead zone in the center of the window. We should not see a click here.
            server.fake_event_hub->synthesize_event(mis::a_motion_event().with_movement(49, 49));
            server.fake_event_hub->synthesize_event(mis::a_button_down_event().of_button(BTN_LEFT).with_action(mis::EventAction::Down));
            server.fake_event_hub->synthesize_event(mis::a_button_up_event().of_button(BTN_LEFT));
            // Now in to the right edge of the window, in the right input region. Again we should see a click
            server.fake_event_hub->synthesize_event(mis::a_motion_event().with_movement(49, 49));
            server.fake_event_hub->synthesize_event(mis::a_button_down_event().of_button(BTN_LEFT).with_action(mis::EventAction::Down));
            server.fake_event_hub->synthesize_event(mis::a_button_up_event().of_button(BTN_LEFT));
        };
    launch_server_process(server_config);

    client_config.expect_cb = [&](MockHandler& handler, mt::WaitCondition& events_received)
        {
            EXPECT_CALL(handler, handle_input(mt::HoverEnterEvent())).Times(AnyNumber());
            EXPECT_CALL(handler, handle_input(mt::HoverExitEvent())).Times(AnyNumber());
            EXPECT_CALL(handler, handle_input(mt::MovementEvent())).Times(AnyNumber());

            {
                // We should see two of the three button pairs.
                InSequence seq;
                EXPECT_CALL(handler, handle_input(mt::ButtonDownEvent(1, 1))).Times(1);
                EXPECT_CALL(handler, handle_input(mt::ButtonUpEvent(1, 1))).Times(1);
                EXPECT_CALL(handler, handle_input(mt::ButtonDownEvent(99, 99))).Times(1);
                EXPECT_CALL(handler, handle_input(mt::ButtonUpEvent(99, 99))).Times(1)
                    .WillOnce(mt::WakeUp(&events_received));
            }
        };
    launch_client_process(client_config);
}

TEST_F(TestClientInput, scene_obscure_motion_events_by_stacking)
{
    static int const screen_width = 100;
    static int const screen_height = 100;

    static geom::Rectangle const screen_geometry{geom::Point{0, 0},
        geom::Size{screen_width, screen_height}};


    auto smaller_geometry = screen_geometry;
    smaller_geometry.size.width = geom::Width{screen_width/2};

    server_config.number_of_clients = 2;
    server_config.produce_events = [&](mtf::InputTestingServerConfiguration& server)
        {
            // First we will move the cursor in to the region where client 2 obscures client 1
            server.fake_event_hub->synthesize_event(mis::a_motion_event().with_movement(1, 1));
            server.fake_event_hub->synthesize_event(mis::a_button_down_event().of_button(BTN_LEFT).with_action(mis::EventAction::Down));
            server.fake_event_hub->synthesize_event(mis::a_button_up_event().of_button(BTN_LEFT));
            // Now we move to the unobscured region of client 1
            server.fake_event_hub->synthesize_event(mis::a_motion_event().with_movement(50, 0));
            server.fake_event_hub->synthesize_event(mis::a_button_down_event().of_button(BTN_LEFT).with_action(mis::EventAction::Down));
            server.fake_event_hub->synthesize_event(mis::a_button_up_event().of_button(BTN_LEFT));
        };
    server_config.client_geometries[test_client_name_1] = screen_geometry;
    server_config.client_geometries[test_client_name_2] = smaller_geometry;
    server_config.client_depths[test_client_name_1] = ms::DepthId{0};
    server_config.client_depths[test_client_name_2] = ms::DepthId{1};

    launch_server_process(server_config);

    client_config_1.expect_cb = [&](MockHandler& handler, mt::WaitCondition& events_received)
        {
            EXPECT_CALL(handler, handle_input(mt::HoverEnterEvent())).Times(AnyNumber());
            EXPECT_CALL(handler, handle_input(mt::HoverExitEvent())).Times(AnyNumber());
            EXPECT_CALL(handler, handle_input(mt::MovementEvent())).Times(AnyNumber());
            {
                // We should only see one button event sequence.
                InSequence seq;
                EXPECT_CALL(handler, handle_input(mt::ButtonDownEvent(51, 1))).Times(1);
                EXPECT_CALL(handler, handle_input(mt::ButtonUpEvent(51, 1))).Times(1)
                    .WillOnce(mt::WakeUp(&events_received));
            }
        };

    client_config_2.expect_cb = [&](MockHandler& handler, mt::WaitCondition& events_received)
        {
            EXPECT_CALL(handler, handle_input(mt::HoverEnterEvent())).Times(AnyNumber());
            EXPECT_CALL(handler, handle_input(mt::HoverExitEvent())).Times(AnyNumber());
            EXPECT_CALL(handler, handle_input(mt::MovementEvent())).Times(AnyNumber());
            {
                // Likewise we should only see one button sequence.
              InSequence seq;
              EXPECT_CALL(handler, handle_input(mt::ButtonDownEvent(1, 1))).Times(1);
              EXPECT_CALL(handler, handle_input(mt::ButtonUpEvent(1, 1))).Times(1)
                  .WillOnce(mt::WakeUp(&events_received));
            }
        };

    launch_client_process(client_config_1);
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
    mtf::CrossProcessSync second_client_done_fence;

    server_config.number_of_clients = 2;
    server_config.produce_events = [&](mtf::InputTestingServerConfiguration& server)
        {
            // We send one event and then hide the surface on top before sending the next.
            // So we expect each of the two surfaces to receive one even
            server.fake_event_hub->synthesize_event(mis::a_motion_event().with_movement(1,1));
            // We use a fence to ensure we do not hide the client
            // before event dispatch occurs
            second_client_done_fence.wait_for_signal_ready_for();

            server.the_session_container()->for_each([&](std::shared_ptr<ms::Session> const& session) -> void
            {
                if (session->name() == test_client_name_2)
                    session->hide();
            });

            server.fake_event_hub->synthesize_event(mis::a_motion_event().with_movement(1,1));
        };
    server_config.client_depths[test_client_name_1] = ms::DepthId{0};
    server_config.client_depths[test_client_name_2] = ms::DepthId{1};
    launch_server_process(server_config);

    client_config_1.expect_cb = [&](MockHandler& handler, mt::WaitCondition& events_received)
        {
            EXPECT_CALL(handler, handle_input(mt::HoverEnterEvent())).Times(AnyNumber());
            EXPECT_CALL(handler, handle_input(mt::HoverExitEvent())).Times(AnyNumber());
            EXPECT_CALL(handler, handle_input(mt::MotionEventWithPosition(2, 2))).Times(1)
                .WillOnce(mt::WakeUp(&events_received));
        };

    client_config_2.expect_cb = [&](MockHandler& handler, mt::WaitCondition& events_received)
        {
            EXPECT_CALL(handler, handle_input(mt::HoverEnterEvent())).Times(AnyNumber());
            EXPECT_CALL(handler, handle_input(mt::HoverExitEvent())).Times(AnyNumber());
            EXPECT_CALL(handler, handle_input(mt::MotionEventWithPosition(1, 1))).Times(1)
                .WillOnce(DoAll(SignalFence(&second_client_done_fence), mt::WakeUp(&events_received)));
        };

    launch_client_process(client_config_1);
    launch_client_process(client_config_2);
}

TEST_F(TestClientInput, clients_receive_motion_within_co_ordinate_system_of_window)
{
    static int const screen_width = 1000;
    static int const screen_height = 800;
    static int const client_height = screen_height/2;
    static int const client_width = screen_width/2;

    server_config.produce_events = [&](mtf::InputTestingServerConfiguration& server)
        {
            server.the_session_container()->for_each([&](std::shared_ptr<ms::Session> const& session) -> void
            {
                session->default_surface()->move_to(geom::Point{screen_width/2-40, screen_height/2-80});
            });
            server.fake_event_hub->synthesize_event(mis::a_motion_event().with_movement(screen_width/2+40, screen_height/2+90));
        };
    server_config.client_geometries[arbitrary_client_name] ={{screen_width/2, screen_height/2}, {client_width, client_height}};
    launch_server_process(server_config);

    client_config.expect_cb = [&](MockHandler& handler, mt::WaitCondition& events_received)
        {
             InSequence seq;
             EXPECT_CALL(handler, handle_input(mt::HoverEnterEvent())).Times(1);
             EXPECT_CALL(handler, handle_input(mt::MotionEventWithPosition(80, 170))).Times(AnyNumber())
                 .WillOnce(mt::WakeUp(&events_received));
        };

    launch_client_process(client_config);
}
