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
 *              Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#include "mir/scene/surface_creation_parameters.h"
#include "mir/scene/placement_strategy.h"
#include "mir/scene/surface.h"
#include "src/server/scene/session_container.h"
#include "mir/scene/session.h"
#include "mir/shell/surface_coordinator_wrapper.h"

#include "mir_test/fake_event_hub.h"
#include "mir_test/wait_condition.h"
#include "mir_test/client_event_matchers.h"
#include "mir_test/barrier.h"
#include "mir_test_framework/deferred_in_process_server.h"
#include "mir_test_framework/input_testing_server_configuration.h"
#include "mir_test_framework/input_testing_client_configuration.h"
#include "mir_test_framework/declarative_placement_strategy.h"
#include "mir_test_framework/using_stub_client_platform.h"

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
    mt::Barrier& input_cb_setup_fence;

    static geom::Rectangle const display_bounds;

    std::function<void(mtf::InputTestingServerConfiguration& server)> produce_events;
    mtf::SurfaceGeometries client_geometries;
    mtf::SurfaceDepths client_depths;

    ServerConfiguration(mt::Barrier& input_cb_setup_fence) : 
        InputTestingServerConfiguration({display_bounds}),
        input_cb_setup_fence(input_cb_setup_fence)
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
        input_cb_setup_fence.ready();
        produce_events(*this);
    }

    std::function<std::shared_ptr<ms::SurfaceCoordinator>(std::shared_ptr<ms::SurfaceCoordinator> const& wrapped)> scwrapper
        = [](std::shared_ptr<ms::SurfaceCoordinator> const& wrapped) { return wrapped; };

    std::shared_ptr<ms::SurfaceCoordinator>
    wrap_surface_coordinator(std::shared_ptr<ms::SurfaceCoordinator> const& wrapped) override
    {
        return scwrapper(wrapped);
    }
};

geom::Rectangle const ServerConfiguration::display_bounds = {{0, 0}, {1600, 1600}};

struct ClientConfig : mtf::InputTestingClientConfiguration
{
    std::function<void(MockInputHandler&, mt::WaitCondition&)> expect_cb;

    ClientConfig(std::string const& client_name, mt::Barrier& client_ready_fence)
        : InputTestingClientConfiguration(client_name, client_ready_fence)
    {
    }

    void tear_down() { if (thread.joinable()) thread.join(); }

    void expect_input(MockInputHandler &handler, mt::WaitCondition& events_received) override
    {
        expect_cb(handler, events_received);
    }

    void exec() { thread = std::thread([this]{ mtf::InputTestingClientConfiguration::exec(); }); }

private:
    std::thread thread;
};

struct TestClientInput : mtf::DeferredInProcessServer
{
    std::string const arbitrary_client_name{"input-test-client"};
    std::string const test_client_name_1 = "1";
    std::string const test_client_name_2 = "2";

    mt::Barrier fence{2};
    mt::WaitCondition second_client_done;
    ServerConfiguration server_configuration{fence};
    mtf::UsingStubClientPlatform using_stub_client_platform;

    mir::DefaultServerConfiguration& server_config() override { return server_configuration; }

    ClientConfig client_config{arbitrary_client_name, fence};
    ClientConfig client_config_1{test_client_name_1, fence};
    ClientConfig client_config_2{test_client_name_2, fence};

    void start_server()
    {
        DeferredInProcessServer::start_server();
        server_configuration.exec();
    }

    void start_client(mtf::InputTestingClientConfiguration& config)
    {
        config.connect_string = new_connection();
        config.exec();
    }

    void TearDown()
    {
        client_config.tear_down();
        client_config_1.tear_down();
        client_config_2.tear_down();
        server_configuration.on_exit();
        DeferredInProcessServer::TearDown();
    }
};
}

using namespace ::testing;
using MockHandler = mtf::InputTestingClientConfiguration::MockInputHandler;


TEST_F(TestClientInput, clients_receive_key_input)
{
    server_configuration.produce_events = [&](mtf::InputTestingServerConfiguration& server)
         {
             int const num_events_produced = 3;

             for (int i = 0; i < num_events_produced; i++)
                 server.fake_event_hub->synthesize_event(mis::a_key_down_event()
                                                         .of_scancode(KEY_ENTER));
         };

    start_server();

    client_config.expect_cb = [&](MockHandler& handler, mt::WaitCondition& events_received)
        {
            InSequence seq;

            EXPECT_CALL(handler, handle_input(mt::KeyDownEvent())).Times(2);
            EXPECT_CALL(handler, handle_input(mt::KeyDownEvent())).Times(1)
                .WillOnce(mt::WakeUp(&events_received));

        };
    start_client(client_config);
}

TEST_F(TestClientInput, clients_receive_us_english_mapped_keys)
{
    server_configuration.produce_events = [&](mtf::InputTestingServerConfiguration& server)
        {
            server.fake_event_hub->synthesize_event(mis::a_key_down_event()
                .of_scancode(KEY_LEFTSHIFT));
            server.fake_event_hub->synthesize_event(mis::a_key_down_event()
                .of_scancode(KEY_4));
        };
    start_server();

    client_config.expect_cb =  [&](MockHandler& handler, mt::WaitCondition& events_received)
        {
            InSequence seq;

            EXPECT_CALL(handler, handle_input(AllOf(mt::KeyDownEvent(), mt::KeyOfSymbol(XKB_KEY_Shift_L)))).Times(1);
            EXPECT_CALL(handler, handle_input(AllOf(mt::KeyDownEvent(), mt::KeyOfSymbol(XKB_KEY_dollar)))).Times(1)
                .WillOnce(mt::WakeUp(&events_received));
        };
    start_client(client_config);
}

TEST_F(TestClientInput, clients_receive_motion_inside_window)
{
    server_configuration.produce_events = [&](mtf::InputTestingServerConfiguration& server)
        {
             server.fake_event_hub->synthesize_event(mis::a_motion_event().with_movement(mtf::InputTestingClientConfiguration::surface_width - 1,
                 mtf::InputTestingClientConfiguration::surface_height - 1));
             server.fake_event_hub->synthesize_event(mis::a_motion_event().with_movement(2,2));
        };
    start_server();

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
    start_client(client_config);
}

TEST_F(TestClientInput, clients_receive_button_events_inside_window)
{
    server_configuration.produce_events = [&](mtf::InputTestingServerConfiguration& server)
        {
             server.fake_event_hub->synthesize_event(mis::a_button_down_event()
                 .of_button(BTN_LEFT).with_action(mis::EventAction::Down));
        };
    start_server();

    client_config.expect_cb = [&](MockHandler& handler, mt::WaitCondition& events_received)
        {
            InSequence seq;

            // The cursor starts at (0, 0).
            EXPECT_CALL(handler, handle_input(mt::ButtonDownEvent(0, 0))).Times(1)
                .WillOnce(mt::WakeUp(&events_received));
        };
    start_client(client_config);
}

TEST_F(TestClientInput, multiple_clients_receive_motion_inside_windows)
{
    static int const screen_width = 1000;
    static int const screen_height = 800;
    static int const client_height = screen_height/2;
    static int const client_width = screen_width/2;

    fence.reset(3);
    server_configuration.produce_events = [&](mtf::InputTestingServerConfiguration& server)
        {
            // In the bounds of the first surface
            server.fake_event_hub->synthesize_event(mis::a_motion_event().with_movement(screen_width/2-1, screen_height/2-1));
            // In the bounds of the second surface
            server.fake_event_hub->synthesize_event(mis::a_motion_event().with_movement(screen_width/2, screen_height/2));
        };
    server_configuration.client_geometries[test_client_name_1] = {{0, 0}, {client_width, client_height}};
    server_configuration.client_geometries[test_client_name_2] = {{screen_width/2, screen_height/2}, {client_width, client_height}};
    start_server();

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

    start_client(client_config_1);
    start_client(client_config_2);
}

namespace
{
struct RegionApplyingSurfaceCoordinator : msh::SurfaceCoordinatorWrapper
{
    RegionApplyingSurfaceCoordinator(std::shared_ptr<ms::SurfaceCoordinator> wrapped_coordinator,
        std::initializer_list<geom::Rectangle> const& input_rectangles)
        : msh::SurfaceCoordinatorWrapper(wrapped_coordinator),
          input_rectangles(input_rectangles)
    {
    }

    std::shared_ptr<ms::Surface> add_surface(
        ms::SurfaceCreationParameters const& params,
        ms::Session* session) override
    {
        auto surface = wrapped->add_surface(params, session);

        surface->set_input_region(input_rectangles);

        return surface;
    }

    std::vector<geom::Rectangle> const input_rectangles;
};
}

TEST_F(TestClientInput, clients_do_not_receive_motion_outside_input_region)
{
    static int const screen_width = 100;
    static int const screen_height = 100;

    static std::initializer_list<geom::Rectangle> client_input_regions{
        {geom::Point{0, 0}, {screen_width-80, screen_height}},
        {geom::Point{screen_width-20, 0}, {screen_width-80, screen_height}}
    };

    server_configuration.scwrapper = [&](std::shared_ptr<ms::SurfaceCoordinator> const& wrapped)
        -> std::shared_ptr<ms::SurfaceCoordinator>
        {
            return std::make_shared<RegionApplyingSurfaceCoordinator>(wrapped, client_input_regions);
        };

    server_configuration.produce_events = [&](mtf::InputTestingServerConfiguration& server)
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
    start_server();

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
    start_client(client_config);
}

TEST_F(TestClientInput, scene_obscure_motion_events_by_stacking)
{
    static int const screen_width = 100;
    static int const screen_height = 100;

    static geom::Rectangle const screen_geometry{geom::Point{0, 0},
        geom::Size{screen_width, screen_height}};


    auto smaller_geometry = screen_geometry;
    smaller_geometry.size.width = geom::Width{screen_width/2};

    fence.reset(3);
    server_configuration.produce_events = [&](mtf::InputTestingServerConfiguration& server)
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
    server_configuration.client_geometries[test_client_name_1] = screen_geometry;
    server_configuration.client_geometries[test_client_name_2] = smaller_geometry;
    server_configuration.client_depths[test_client_name_1] = ms::DepthId{0};
    server_configuration.client_depths[test_client_name_2] = ms::DepthId{1};

    start_server();

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

    start_client(client_config_1);
    start_client(client_config_2);
}

namespace
{

ACTION_P(SignalFence, fence)
{
    fence->wake_up_everyone();
}

}

TEST_F(TestClientInput, hidden_clients_do_not_receive_pointer_events)
{
    fence.reset(3);
    server_configuration.produce_events = [&](mtf::InputTestingServerConfiguration& server)
        {
            // We send one event and then hide the surface on top before sending the next.
            // So we expect each of the two surfaces to receive one even
            server.fake_event_hub->synthesize_event(mis::a_motion_event().with_movement(1,1));
            // We use a fence to ensure we do not hide the client
            // before event dispatch occurs
            second_client_done.wait_for_at_most_seconds(60);

            server.the_session_container()->for_each([&](std::shared_ptr<ms::Session> const& session) -> void
            {
                if (session->name() == test_client_name_2)
                    session->hide();
            });

            server.fake_event_hub->synthesize_event(mis::a_motion_event().with_movement(1,1));
        };
    server_configuration.client_depths[test_client_name_1] = ms::DepthId{0};
    server_configuration.client_depths[test_client_name_2] = ms::DepthId{1};
    start_server();

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
                .WillOnce(DoAll(SignalFence(&second_client_done), mt::WakeUp(&events_received)));
        };

    start_client(client_config_1);
    start_client(client_config_2);
}

TEST_F(TestClientInput, clients_receive_motion_within_co_ordinate_system_of_window)
{
    static int const screen_width = 1000;
    static int const screen_height = 800;
    static int const client_height = screen_height/2;
    static int const client_width = screen_width/2;

    server_configuration.produce_events = [&](mtf::InputTestingServerConfiguration& server)
        {
            server.the_session_container()->for_each([&](std::shared_ptr<ms::Session> const& session) -> void
            {
                session->default_surface()->move_to(geom::Point{screen_width/2-40, screen_height/2-80});
            });
            server.fake_event_hub->synthesize_event(mis::a_motion_event().with_movement(screen_width/2+40, screen_height/2+90));
        };
    server_configuration.client_geometries[arbitrary_client_name] ={{screen_width/2, screen_height/2}, {client_width, client_height}};
    start_server();

    client_config.expect_cb = [&](MockHandler& handler, mt::WaitCondition& events_received)
        {
             InSequence seq;
             EXPECT_CALL(handler, handle_input(mt::HoverEnterEvent())).Times(1);
             EXPECT_CALL(handler, handle_input(mt::MotionEventWithPosition(80, 170))).Times(AnyNumber())
                 .WillOnce(mt::WakeUp(&events_received));
        };

    start_client(client_config);
}

// TODO: Consider tests for more input devices with custom mapping (i.e. joysticks...)
TEST_F(TestClientInput, usb_direct_input_devices_work)
{
    using namespace ::testing;

    auto minimum_touch = mi::android::FakeEventHub::TouchScreenMinAxisValue;
    auto maximum_touch = mi::android::FakeEventHub::TouchScreenMaxAxisValue;
    auto display_width = ServerConfiguration::display_bounds.size.width.as_uint32_t();
    auto display_height = ServerConfiguration::display_bounds.size.height.as_uint32_t();

    // We place a click 10% in to the touchscreens space in both axis, and a second at 0,0. Thus we expect to see a click at
    // .1*screen_width/height and a second at zero zero.
    static int const abs_touch_x_1 = minimum_touch+(maximum_touch-minimum_touch)*.10;
    static int const abs_touch_y_1 = minimum_touch+(maximum_touch-minimum_touch)*.10;
    static int const abs_touch_x_2 = 0;
    static int const abs_touch_y_2 = 0;

    static float const expected_scale_x = float(display_width) / (maximum_touch - minimum_touch + 1);
    static float const expected_scale_y = float(display_height) / (maximum_touch - minimum_touch + 1);

    static float const expected_motion_x_1 = expected_scale_x * abs_touch_x_1;
    static float const expected_motion_y_1 = expected_scale_y * abs_touch_y_1;
    static float const expected_motion_x_2 = expected_scale_x * abs_touch_x_2;
    static float const expected_motion_y_2 = expected_scale_y * abs_touch_y_2;

    server_configuration.produce_events = [&](mtf::InputTestingServerConfiguration& server)
        {
            server.fake_event_hub->synthesize_event(mis::a_touch_event().at_position({abs_touch_x_1, abs_touch_y_1}));
            // Sleep here to trigger more failures (simulate slow machine)
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            server.fake_event_hub->synthesize_event(mis::a_touch_event().at_position({abs_touch_x_2, abs_touch_y_2}));
        };
    server_configuration.client_geometries[arbitrary_client_name] = ServerConfiguration::display_bounds;
    start_server();

    client_config.expect_cb = [&](MockHandler& handler, mt::WaitCondition& events_received)
        {
            InSequence seq;
            EXPECT_CALL(handler, handle_input(
                mt::TouchEvent(expected_motion_x_1, expected_motion_y_1))).Times(1);
            EXPECT_CALL(handler, handle_input(
                mt::MotionEventInDirection(expected_motion_x_1,
                                           expected_motion_y_1,
                                           expected_motion_x_2,
                                           expected_motion_y_2)))
                .Times(1)
                .WillOnce(mt::WakeUp(&events_received));
        };
    start_client(client_config);
}

TEST_F(TestClientInput, send_mir_input_events_through_surface)
{
    MirEvent key_event;
    std::memset(&key_event, 0, sizeof key_event);
    key_event.type = mir_event_type_key;
    key_event.key.action= mir_key_action_down;

    server_configuration.produce_events = [key_event](mtf::InputTestingServerConfiguration& server)
         {
             server.the_session_container()->for_each([key_event](std::shared_ptr<ms::Session> const& session) -> void
                {
                    session->default_surface()->consume(key_event);
                });
         };

    start_server();

    client_config.expect_cb = [&](MockHandler& handler, mt::WaitCondition& events_received)
    {
        EXPECT_CALL(handler, handle_input(mt::KeyDownEvent())).Times(1)
                 .WillOnce(mt::WakeUp(&events_received));
    };
    start_client(client_config);
}
