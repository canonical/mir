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
#include "src/server/scene/session_container.h"
#include "mir/shell/session.h"
#include "src/server/scene/surface_controller.h"
#include "src/server/scene/surface_stack_model.h"

#include "src/server/input/android/android_input_manager.h"
#include "src/server/input/android/android_input_targeter.h"

#include "mir_toolkit/mir_client_library.h"

#include "mir_test/fake_shared.h"
#include "mir_test/fake_event_hub.h"
#include "mir_test/event_factory.h"
#include "mir_test/wait_condition.h"
#include "mir_test/client_event_matchers.h"
#include "mir_test_framework/cross_process_sync.h"
#include "mir_test_framework/display_server_test_fixture.h"
#include "mir_test_framework/input_testing_server_configuration.h"
#include "mir_test_framework/input_testing_client_configuration.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <thread>
#include <functional>
#include <map>

#include <poll.h>

namespace mi = mir::input;
namespace mia = mi::android;
namespace mis = mi::synthesis;
namespace mf = mir::frontend;
namespace msh = mir::shell;
namespace ms = mir::scene;
namespace mg = mir::graphics;
namespace geom = mir::geometry;
namespace mt = mir::test;
namespace mtd = mt::doubles;
namespace mtf = mir_test_framework;

namespace
{
typedef std::map<std::string, geom::Rectangle> GeometryMap;
typedef std::map<std::string, ms::DepthId> DepthMap;

struct StaticPlacementStrategy : public msh::PlacementStrategy
{
    StaticPlacementStrategy(std::shared_ptr<msh::PlacementStrategy> const& underlying_strategy,
                            GeometryMap const& positions,
                            DepthMap const& depths)
        : underlying_strategy(underlying_strategy),
          surface_geometry_by_name(positions),
          surface_depths_by_name(depths)
    {
    }

    StaticPlacementStrategy(std::shared_ptr<msh::PlacementStrategy> const& underlying_strategy,
                            GeometryMap const& positions)
        : StaticPlacementStrategy(underlying_strategy, positions, DepthMap())
    {
    }

    msh::SurfaceCreationParameters place(msh::Session const& session, msh::SurfaceCreationParameters const& request_parameters)
    {
        auto placed = request_parameters;
        auto const& name = request_parameters.name;

        auto it = surface_geometry_by_name.find(name);
        if (it != surface_geometry_by_name.end())
        {
            auto const& geometry = it->second;
            placed.top_left = geometry.top_left;
            placed.size = geometry.size;
        }
        else
        {
            placed = underlying_strategy->place(session, placed);
        }
        placed.depth = surface_depths_by_name[name];

        return placed;
    }

    std::shared_ptr<msh::PlacementStrategy> const underlying_strategy;
    GeometryMap surface_geometry_by_name;
    DepthMap surface_depths_by_name;
};

std::shared_ptr<mtf::InputTestingServerConfiguration>
make_event_producing_server(mtf::CrossProcessSync const& client_ready_fence,
    int number_of_clients,
    std::function<void(mtf::InputTestingServerConfiguration& server)> const& produce_events,
    GeometryMap const& client_geometry_map, DepthMap const& client_depth_map)
{
    struct ServerConfiguration : mtf::InputTestingServerConfiguration
    {
        mtf::CrossProcessSync input_cb_setup_fence;
        int const number_of_clients;
        std::function<void(mtf::InputTestingServerConfiguration& server)> const produce_events;
        GeometryMap const client_geometry;
        DepthMap const client_depth;

        ServerConfiguration(mtf::CrossProcessSync const& input_cb_setup_fence, int number_of_clients,
                            std::function<void(mtf::InputTestingServerConfiguration& server)> const& produce_events,
                            GeometryMap const& client_geometry, DepthMap const& client_depth)
                : input_cb_setup_fence(input_cb_setup_fence),
                  number_of_clients(number_of_clients),
                  produce_events(produce_events),
                  client_geometry(client_geometry),
                  client_depth(client_depth)
        {
        }

        std::shared_ptr<msh::PlacementStrategy> the_shell_placement_strategy() override
        {
            return std::make_shared<StaticPlacementStrategy>(InputTestingServerConfiguration::the_shell_placement_strategy(),
                client_geometry, client_depth);
        }

        void inject_input()
        {
            for (int i = 1; i < number_of_clients + 1; i++)
                EXPECT_EQ(i, input_cb_setup_fence.wait_for_signal_ready_for());
            produce_events(*this);
        }
    };
    return std::make_shared<ServerConfiguration>(client_ready_fence, number_of_clients,
        produce_events, client_geometry_map, client_depth_map);
}

std::shared_ptr<mtf::InputTestingServerConfiguration>
make_event_producing_server(mtf::CrossProcessSync const& client_ready_fence, int number_of_clients,
                            std::function<void(mtf::InputTestingServerConfiguration& server)> const& produce_events)
{
    return make_event_producing_server(client_ready_fence, number_of_clients,
        produce_events, GeometryMap(), DepthMap());
}

std::shared_ptr<mtf::InputTestingClientConfiguration>
make_event_expecting_client(std::string const& client_name, mtf::CrossProcessSync const& client_ready_fence,
                            std::function<void(mtf::InputTestingClientConfiguration::MockInputHandler &, mt::WaitCondition&)> const& expect_input)
{
    struct EventReceivingClient : mtf::InputTestingClientConfiguration
    {
        std::function<void(MockInputHandler&, mt::WaitCondition&)> const expect_cb;

        EventReceivingClient(std::string const& client_name, mtf::CrossProcessSync const& client_ready_fence,
                             std::function<void(MockInputHandler&, mt::WaitCondition&)> const& expect_cb)
            : InputTestingClientConfiguration(client_name, client_ready_fence),
              expect_cb(expect_cb)
        {
        }
        void expect_input(MockInputHandler &handler, mt::WaitCondition& events_received) override
        {
            expect_cb(handler, events_received);
        }
    };
    return std::make_shared<EventReceivingClient>(client_name, client_ready_fence, expect_input);
}

std::shared_ptr<mtf::InputTestingClientConfiguration>
make_event_expecting_client(mtf::CrossProcessSync const& client_ready_fence,
                            std::function<void(mtf::InputTestingClientConfiguration::MockInputHandler &, mt::WaitCondition&)> const& expect_input)
{
    return make_event_expecting_client("input-test-client", client_ready_fence, expect_input);
}

}


using TestClientInput = BespokeDisplayServerTestFixture;
using MockHandler = mtf::InputTestingClientConfiguration::MockInputHandler;

TEST_F(TestClientInput, clients_receive_key_input)
{
    using namespace ::testing;

    static std::string const test_client_name = "1";

    mtf::CrossProcessSync fence;

    auto server_config = make_event_producing_server(fence, 1,
         [&](mtf::InputTestingServerConfiguration& server)
         {
             int const num_events_produced = 3;

             for (int i = 0; i < num_events_produced; i++)
                 server.fake_event_hub->synthesize_event(mis::a_key_down_event()
                                                         .of_scancode(KEY_ENTER));
         });
    launch_server_process(*server_config);

    auto client_config = make_event_expecting_client(fence,
         [&](MockHandler& handler, mt::WaitCondition& events_received)
         {
            using namespace ::testing;
            InSequence seq;

            EXPECT_CALL(handler, handle_input(mt::KeyDownEvent())).Times(2);
            EXPECT_CALL(handler, handle_input(mt::KeyDownEvent())).Times(1)
                .WillOnce(mt::WakeUp(&events_received));

         });
    launch_client_process(*client_config);
}

TEST_F(TestClientInput, clients_receive_us_english_mapped_keys)
{
    using namespace ::testing;
    static std::string const test_client_name = "1";
    mtf::CrossProcessSync fence;

    auto server_config = make_event_producing_server(fence, 1,
         [&](mtf::InputTestingServerConfiguration& server)
         {
            server.fake_event_hub->synthesize_event(mis::a_key_down_event()
                .of_scancode(KEY_LEFTSHIFT));
            server.fake_event_hub->synthesize_event(mis::a_key_down_event()
                .of_scancode(KEY_4));
        });
    launch_server_process(*server_config);

    auto client_config = make_event_expecting_client(fence,
         [&](MockHandler& handler, mt::WaitCondition& events_received)
         {
            using namespace ::testing;
            InSequence seq;

            EXPECT_CALL(handler, handle_input(AllOf(mt::KeyDownEvent(), mt::KeyOfSymbol(XKB_KEY_Shift_L)))).Times(1);
            EXPECT_CALL(handler, handle_input(AllOf(mt::KeyDownEvent(), mt::KeyOfSymbol(XKB_KEY_dollar)))).Times(1)
                .WillOnce(mt::WakeUp(&events_received));
        });
    launch_client_process(*client_config);
}

TEST_F(TestClientInput, clients_receive_motion_inside_window)
{
    using namespace ::testing;
    static std::string const test_client_name = "1";
    mtf::CrossProcessSync fence;

    auto server_config = make_event_producing_server(fence, 1,
         [&](mtf::InputTestingServerConfiguration& server)
         {
             server.fake_event_hub->synthesize_event(mis::a_motion_event().with_movement(mtf::InputTestingClientConfiguration::surface_width - 1,
                 mtf::InputTestingClientConfiguration::surface_height - 1));
             server.fake_event_hub->synthesize_event(mis::a_motion_event().with_movement(2,2));
        });
    launch_server_process(*server_config);

    auto client_config = make_event_expecting_client(fence,
         [&](MockHandler& handler, mt::WaitCondition& events_received)
         {
            using namespace ::testing;
            InSequence seq;

            // We should see the cursor enter
            EXPECT_CALL(handler, handle_input(mt::HoverEnterEvent())).Times(1);
            EXPECT_CALL(handler, handle_input(
                mt::MotionEventWithPosition(mtf::InputTestingClientConfiguration::surface_width - 1,
                                        mtf::InputTestingClientConfiguration::surface_height - 1))).Times(1)
                .WillOnce(mt::WakeUp(&events_received));
            // But we should not receive an event for the second movement outside of our surface!
         });
    launch_client_process(*client_config);
}

TEST_F(TestClientInput, clients_receive_button_events_inside_window)
{
    using namespace ::testing;

    static std::string const test_client_name = "1";
    mtf::CrossProcessSync fence;

    auto server_config = make_event_producing_server(fence, 1,
         [&](mtf::InputTestingServerConfiguration& server)
         {
             server.fake_event_hub->synthesize_event(mis::a_button_down_event()
                 .of_button(BTN_LEFT).with_action(mis::EventAction::Down));
         });
    launch_server_process(*server_config);

    auto client_config = make_event_expecting_client(fence,
         [&](MockHandler& handler, mt::WaitCondition& events_received)
         {
            using namespace ::testing;
            InSequence seq;

            // The cursor starts at (0, 0).
            EXPECT_CALL(handler, handle_input(mt::ButtonDownEvent(0, 0))).Times(1)
                .WillOnce(mt::WakeUp(&events_received));
        });
    launch_client_process(*client_config);
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

    static GeometryMap positions;
    positions[test_client_1] = geom::Rectangle{geom::Point{0, 0},
                                               geom::Size{client_width, client_height}};
    positions[test_client_2] = geom::Rectangle{geom::Point{screen_width/2, screen_height/2},
                                               geom::Size{client_width, client_height}};

    auto server_config = make_event_producing_server(fence, 2,
         [&](mtf::InputTestingServerConfiguration& server)
         {
            // In the bounds of the first surface
            server.fake_event_hub->synthesize_event(mis::a_motion_event().with_movement(screen_width/2-1, screen_height/2-1));
            // In the bounds of the second surface
            server.fake_event_hub->synthesize_event(mis::a_motion_event().with_movement(screen_width/2, screen_height/2));
        }, positions, DepthMap());
    launch_server_process(*server_config);

    auto client_1 = make_event_expecting_client(test_client_1, fence,
         [&](MockHandler& handler, mt::WaitCondition& events_received)
         {
            InSequence seq;
            EXPECT_CALL(handler, handle_input(mt::HoverEnterEvent())).Times(1);
            EXPECT_CALL(handler, handle_input(mt::MotionEventWithPosition(client_width - 1, client_height - 1))).Times(1);
            EXPECT_CALL(handler, handle_input(mt::HoverExitEvent())).Times(1)
                .WillOnce(mt::WakeUp(&events_received));
        });
    auto client_2 = make_event_expecting_client(test_client_2, fence,
         [&](MockHandler& handler, mt::WaitCondition& events_received)
         {
             InSequence seq;
             EXPECT_CALL(handler, handle_input(mt::HoverEnterEvent())).Times(1);
             EXPECT_CALL(handler, handle_input(mt::MotionEventWithPosition(client_width - 1, client_height - 1))).Times(1)
                 .WillOnce(mt::WakeUp(&events_received));
        });

    launch_client_process(*client_1);
    launch_client_process(*client_2);
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

            return std::make_shared<StaticPlacementStrategy>(InputTestingServerConfiguration::the_shell_placement_strategy(), positions);
        }
        std::shared_ptr<msh::SurfaceFactory> the_shell_surface_factory() override
        {
            return std::make_shared<RegionApplyingSurfaceFactory>(InputTestingServerConfiguration::the_shell_surface_factory(),
                client_input_regions);
        }

        void inject_input() override
        {
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

    auto client_config = make_event_expecting_client(test_client_name, fence,
         [&](MockHandler& handler, mt::WaitCondition& events_received)
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
        });
    launch_client_process(*client_config);
}

TEST_F(TestClientInput, scene_obscure_motion_events_by_stacking)
{
    using namespace ::testing;

    static std::string const test_client_name_1 = "1";
    static std::string const test_client_name_2 = "2";
    mtf::CrossProcessSync fence;

    static int const screen_width = 100;
    static int const screen_height = 100;

    static geom::Rectangle const screen_geometry{geom::Point{0, 0},
        geom::Size{screen_width, screen_height}};

    static GeometryMap positions;
    positions[test_client_name_1] = screen_geometry;

    auto smaller_geometry = screen_geometry;
    smaller_geometry.size.width = geom::Width{screen_width/2};
    positions[test_client_name_2] = smaller_geometry;

    static DepthMap depths;
    depths[test_client_name_1] = ms::DepthId{0};
    depths[test_client_name_2] = ms::DepthId{1};

    auto server_config = make_event_producing_server(fence, 2,
        [&](mtf::InputTestingServerConfiguration& server)
        {
            // First we will move the cursor in to the region where client 2 obscures client 1
            server.fake_event_hub->synthesize_event(mis::a_motion_event().with_movement(1, 1));
            server.fake_event_hub->synthesize_event(mis::a_button_down_event().of_button(BTN_LEFT).with_action(mis::EventAction::Down));
            server.fake_event_hub->synthesize_event(mis::a_button_up_event().of_button(BTN_LEFT));
            // Now we move to the unobscured region of client 1
            server.fake_event_hub->synthesize_event(mis::a_motion_event().with_movement(50, 0));
            server.fake_event_hub->synthesize_event(mis::a_button_down_event().of_button(BTN_LEFT).with_action(mis::EventAction::Down));
            server.fake_event_hub->synthesize_event(mis::a_button_up_event().of_button(BTN_LEFT));
        }, positions, depths);
    launch_server_process(*server_config);

    auto client_config_1 = make_event_expecting_client(test_client_name_1, fence,
         [&](MockHandler& handler, mt::WaitCondition& events_received)
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
        });
    auto client_config_2 = make_event_expecting_client(test_client_name_2, fence,
         [&](MockHandler& handler, mt::WaitCondition& events_received)
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
        });

    launch_client_process(*client_config_1);
    launch_client_process(*client_config_2);
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

    static DepthMap depths;
    depths[test_client_name] = ms::DepthId{0};
    depths[test_client_2_name] = ms::DepthId{1};

    auto server_config = make_event_producing_server(fence, 2,
        [&](mtf::InputTestingServerConfiguration& server)
        {
            // We send one event and then hide the surface on top before sending the next.
            // So we expect each of the two surfaces to receive one even
            server.fake_event_hub->synthesize_event(mis::a_motion_event().with_movement(1,1));
            // We use a fence to ensure we do not hide the client
            // before event dispatch occurs
            second_client_done_fence.wait_for_signal_ready_for();

            server.the_session_container()->for_each([&](std::shared_ptr<msh::Session> const& session) -> void
            {
                if (session->name() == test_client_2_name)
                    session->hide();
            });

            server.fake_event_hub->synthesize_event(mis::a_motion_event().with_movement(1,1));
        }, GeometryMap(), depths);
    launch_server_process(*server_config);

    auto client_config_1 = make_event_expecting_client(test_client_name, fence,
         [&](MockHandler& handler, mt::WaitCondition& events_received)
         {
            EXPECT_CALL(handler, handle_input(mt::HoverEnterEvent())).Times(AnyNumber());
            EXPECT_CALL(handler, handle_input(mt::HoverExitEvent())).Times(AnyNumber());
            EXPECT_CALL(handler, handle_input(mt::MotionEventWithPosition(2, 2))).Times(1)
                .WillOnce(mt::WakeUp(&events_received));
        });
    auto client_config_2 = make_event_expecting_client(test_client_2_name, fence,
         [&](MockHandler& handler, mt::WaitCondition& events_received)
         {
            EXPECT_CALL(handler, handle_input(mt::HoverEnterEvent())).Times(AnyNumber());
            EXPECT_CALL(handler, handle_input(mt::HoverExitEvent())).Times(AnyNumber());
            EXPECT_CALL(handler, handle_input(mt::MotionEventWithPosition(1, 1))).Times(1)
                .WillOnce(DoAll(SignalFence(&second_client_done_fence), mt::WakeUp(&events_received)));
        });

    launch_client_process(*client_config_1);
    launch_client_process(*client_config_2);
}

TEST_F(TestClientInput, clients_receive_motion_within_co_ordinate_system_of_window)
{
    using namespace ::testing;

    static int const screen_width = 1000;
    static int const screen_height = 800;
    static int const client_height = screen_height/2;
    static int const client_width = screen_width/2;
    static std::string const test_client = "tc";
    mtf::CrossProcessSync fence;

    static GeometryMap positions;
    positions[test_client] = geom::Rectangle{geom::Point{screen_width/2, screen_height/2},
                                             geom::Size{client_width, client_height}};

    auto server_config = make_event_producing_server(fence, 1,
         [&](mtf::InputTestingServerConfiguration& server)
         {
            server.the_session_container()->for_each([&](std::shared_ptr<msh::Session> const& session) -> void
            {
                session->default_surface()->move_to(geom::Point{screen_width/2-40, screen_height/2-80});
            });
            server.fake_event_hub->synthesize_event(mis::a_motion_event().with_movement(screen_width/2+40, screen_height/2+90));
        }, positions, DepthMap());
    launch_server_process(*server_config);

    auto client = make_event_expecting_client(test_client, fence,
         [&](MockHandler& handler, mt::WaitCondition& events_received)
         {
             InSequence seq;
             EXPECT_CALL(handler, handle_input(mt::HoverEnterEvent())).Times(1);
             EXPECT_CALL(handler, handle_input(mt::MotionEventWithPosition(80, 170))).Times(AnyNumber())
                 .WillOnce(mt::WakeUp(&events_received));
        });

    launch_client_process(*client);
}
