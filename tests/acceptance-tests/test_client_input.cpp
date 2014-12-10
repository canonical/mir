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
 *              Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "mir/shell/surface_coordinator_wrapper.h"
#include "mir/scene/surface_creation_parameters.h"
#include "mir/scene/surface.h"
#include "mir/scene/session.h"
#include "src/server/scene/session_container.h"

#include "mir_test_framework/in_process_server.h"
#include "mir_test_framework/using_stub_client_platform.h"
#include "mir_test_framework/fake_event_hub_server_configuration.h"
#include "mir_test_framework/declarative_placement_strategy.h"
#include "mir_test/wait_condition.h"
#include "mir_test/fake_event_hub.h"
#include "mir_test/client_event_matchers.h"
#include "mir_test/spin_wait.h"

#include "mir_toolkit/mir_client_library.h"

#include <linux/input.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <cstring>

namespace mt = mir::test;
namespace mtf = mir_test_framework;
namespace mis = mir::input::synthesis;
namespace mia = mir::input::android;
namespace msh = mir::shell;
namespace ms = mir::scene;
namespace geom = mir::geometry;

namespace
{

struct MockInputHandler
{
    MOCK_METHOD1(handle_input, void(MirEvent const*));
};

struct InputClient
{
    InputClient(std::string const& connect_string, std::string const& client_name)
        : connect_string{connect_string}, client_name{client_name}
    {
    }


    ~InputClient()
    {
        if (client_thread.joinable())
            client_thread.join();
    }

    void start()
    {
        client_thread = std::thread{[this] { run(); }};
        ready_to_accept_events.wait_for_at_most_seconds(5);
    }

    void run()
    {
        auto const connection =
            mir_connect_sync(connect_string.c_str(), client_name.c_str());

        auto spec = mir_connection_create_spec_for_normal_surface(connection, surface_width,
            surface_height, mir_pixel_format_bgr_888);
        mir_surface_spec_set_name(spec, client_name.c_str());
        auto surface = mir_surface_create_sync(spec);
        mir_surface_spec_release(spec);

        MirEventDelegate const event_delegate { handle_input, this };
        mir_surface_set_event_handler(surface, &event_delegate);
        mir_surface_swap_buffers_sync(surface);

        wait_for_surface_to_become_focused_and_exposed(surface);

        ready_to_accept_events.wake_up_everyone();
        all_events_received.wait_for_at_most_seconds(10);

        mir_surface_release_sync(surface);
        mir_connection_release(connection);
    }

    static void handle_input(MirSurface*, MirEvent const* ev, void* context)
    {
        auto const client = static_cast<InputClient*>(context);

        if (ev->type == mir_event_type_surface)
            return;

        client->handler.handle_input(ev);
    }

    void wait_for_surface_to_become_focused_and_exposed(MirSurface* surface)
    {
        bool success = mt::spin_wait_for_condition_or_timeout(
            [surface]
            {
                return mir_surface_get_visibility(surface) == mir_surface_visibility_exposed &&
                       mir_surface_get_focus(surface) == mir_surface_focused;
            },
            std::chrono::seconds{5});

        if (!success)
            throw std::runtime_error("Timeout waiting for surface to become focused and exposed");
    }

    static int const surface_width = 100;
    static int const surface_height = 100;

    std::string const connect_string;
    std::string const client_name;

    std::thread client_thread;
    MockInputHandler handler;
    mir::test::WaitCondition all_events_received;
    mir::test::WaitCondition ready_to_accept_events;
};

using ClientInputRegions = std::map<std::string, std::vector<geom::Rectangle>>;

struct RegionApplyingSurfaceCoordinator : msh::SurfaceCoordinatorWrapper
{
    RegionApplyingSurfaceCoordinator(
        std::shared_ptr<ms::SurfaceCoordinator> wrapped_coordinator,
        ClientInputRegions const& client_input_regions)
        : msh::SurfaceCoordinatorWrapper(wrapped_coordinator),
          client_input_regions(client_input_regions)
    {
    }

    std::shared_ptr<ms::Surface> add_surface(
        ms::SurfaceCreationParameters const& params,
        ms::Session* session) override
    {
        auto const surface = wrapped->add_surface(params, session);

        if (client_input_regions.find(params.name) != client_input_regions.end())
            surface->set_input_region(client_input_regions.at(params.name));

        return surface;
    }

    ClientInputRegions const& client_input_regions;
};

struct TestServerConfiguration : mtf::FakeEventHubServerConfiguration
{
    TestServerConfiguration(geom::Rectangle const& screen_geometry)
        : mtf::FakeEventHubServerConfiguration(
              std::vector<geom::Rectangle>{screen_geometry})
    {
    }

    std::shared_ptr<mir::scene::PlacementStrategy> the_placement_strategy() override
    {
        return std::make_shared<mtf::DeclarativePlacementStrategy>(
            FakeEventHubServerConfiguration::the_placement_strategy(),
            client_geometries, client_depths);
    }

    std::shared_ptr<ms::SurfaceCoordinator>
    wrap_surface_coordinator(std::shared_ptr<ms::SurfaceCoordinator> const& wrapped) override
    {
        return std::make_shared<RegionApplyingSurfaceCoordinator>(
            wrapped,
            client_input_regions);
    }

    mtf::SurfaceGeometries client_geometries;
    mtf::SurfaceDepths client_depths;
    ClientInputRegions client_input_regions;
};

struct TestClientInput : mtf::InProcessServer
{
    mir::DefaultServerConfiguration& server_config() override
    {
        return server_configuration_;
    }

    TestServerConfiguration& test_server_config()
    {
        return server_configuration_;
    }

    std::shared_ptr<mir::input::android::FakeEventHub> fake_event_hub()
    {
        return server_configuration_.fake_event_hub;
    }

    std::string const test_client_name_1{"client1"};
    std::string const test_client_name_2{"client2"};
    geom::Rectangle const screen_geometry{{0, 0}, {1000, 800}};
    TestServerConfiguration server_configuration_{screen_geometry};
    mtf::UsingStubClientPlatform using_stub_client_platform;
};

}

TEST_F(TestClientInput, clients_receive_key_input)
{
    using namespace testing;

    InputClient client{new_connection(), test_client_name_1};

    EXPECT_CALL(client.handler, handle_input(mt::KeyDownEvent()))
        .WillOnce(Return())
        .WillOnce(Return())
        .WillOnce(mt::WakeUp(&client.all_events_received));

    int const num_events_produced = 3;

    client.start();

    for (int i = 0; i < num_events_produced; i++)
        fake_event_hub()->synthesize_event(
            mis::a_key_down_event().of_scancode(KEY_ENTER));
}

TEST_F(TestClientInput, clients_receive_us_english_mapped_keys)
{
    using namespace testing;

    InputClient client{new_connection(), test_client_name_1};

    InSequence seq;

    EXPECT_CALL(client.handler,
                handle_input(
                    AllOf(mt::KeyDownEvent(), mt::KeyOfSymbol(XKB_KEY_Shift_L))));
    EXPECT_CALL(client.handler,
                handle_input(
                    AllOf(mt::KeyDownEvent(), mt::KeyOfSymbol(XKB_KEY_dollar))))
        .WillOnce(mt::WakeUp(&client.all_events_received));

    client.start();

    fake_event_hub()->synthesize_event(
        mis::a_key_down_event().of_scancode(KEY_LEFTSHIFT));
    fake_event_hub()->synthesize_event(
        mis::a_key_down_event().of_scancode(KEY_4));
}

TEST_F(TestClientInput, clients_receive_motion_inside_window)
{
    using namespace testing;

    InputClient client{new_connection(), test_client_name_1};

    InSequence seq;

    // We should see the cursor enter
    EXPECT_CALL(client.handler, handle_input(mt::HoverEnterEvent()));
    EXPECT_CALL(client.handler,
                handle_input(
                    mt::MotionEventWithPosition(
                        InputClient::surface_width - 1,
                        InputClient::surface_height - 1)))
        .WillOnce(mt::WakeUp(&client.all_events_received));
    // But we should not receive an event for the second movement outside of our surface!

    client.start();

    fake_event_hub()->synthesize_event(
        mis::a_motion_event().with_movement(
            InputClient::surface_width - 1,
            InputClient::surface_height - 1));
    fake_event_hub()->synthesize_event(mis::a_motion_event().with_movement(2,2));
}

TEST_F(TestClientInput, clients_receive_button_events_inside_window)
{
    using namespace testing;

    InputClient client{new_connection(), test_client_name_1};

    // The cursor starts at (0, 0).
    EXPECT_CALL(client.handler, handle_input(mt::ButtonDownEvent(0, 0)))
        .WillOnce(mt::WakeUp(&client.all_events_received));

    client.start();

    fake_event_hub()->synthesize_event(
        mis::a_button_down_event()
            .of_button(BTN_LEFT)
            .with_action(mis::EventAction::Down));
}

TEST_F(TestClientInput, multiple_clients_receive_motion_inside_windows)
{
    using namespace testing;

    int const screen_width = screen_geometry.size.width.as_int();
    int const screen_height = screen_geometry.size.height.as_int();
    int const client_height = screen_height / 2;
    int const client_width = screen_width / 2;

    test_server_config().client_geometries[test_client_name_1] =
        {{0, 0}, {client_width, client_height}};
    test_server_config().client_geometries[test_client_name_2] =
        {{screen_width / 2, screen_height / 2}, {client_width, client_height}};

    InputClient client1{new_connection(), test_client_name_1};
    InputClient client2{new_connection(), test_client_name_2};

    {
        InSequence seq;
        EXPECT_CALL(client1.handler, handle_input(mt::HoverEnterEvent()));
        EXPECT_CALL(client1.handler,
                    handle_input(
                        mt::MotionEventWithPosition(client_width - 1, client_height - 1)));
        EXPECT_CALL(client1.handler, handle_input(mt::HoverExitEvent()))
            .WillOnce(mt::WakeUp(&client1.all_events_received));
    }

    {
        InSequence seq;
        EXPECT_CALL(client2.handler, handle_input(mt::HoverEnterEvent()));
        EXPECT_CALL(client2.handler,
                    handle_input(
                        mt::MotionEventWithPosition(client_width - 1, client_height - 1)))
            .WillOnce(mt::WakeUp(&client2.all_events_received));
    }

    client1.start();
    client2.start();

    // In the bounds of the first surface
    fake_event_hub()->synthesize_event(
        mis::a_motion_event().with_movement(screen_width / 2 - 1, screen_height / 2 - 1));
    // In the bounds of the second surface
    fake_event_hub()->synthesize_event(
        mis::a_motion_event().with_movement(screen_width / 2, screen_height / 2));
}

TEST_F(TestClientInput, clients_do_not_receive_motion_outside_input_region)
{
    using namespace testing;

    int const client_height = InputClient::surface_height;
    int const client_width = InputClient::surface_width;

    test_server_config().client_input_regions[test_client_name_1] = {
        {{0, 0}, {client_width - 80, client_height}},
        {{client_width - 20, 0}, {client_width - 80, client_height}}};

    InputClient client{new_connection(), test_client_name_1};

    EXPECT_CALL(client.handler, handle_input(mt::HoverEnterEvent())).Times(AnyNumber());
    EXPECT_CALL(client.handler, handle_input(mt::HoverExitEvent())).Times(AnyNumber());
    EXPECT_CALL(client.handler, handle_input(mt::MovementEvent())).Times(AnyNumber());

    {
        // We should see two of the three button pairs.
        InSequence seq;
        EXPECT_CALL(client.handler, handle_input(mt::ButtonDownEvent(1, 1)));
        EXPECT_CALL(client.handler, handle_input(mt::ButtonUpEvent(1, 1)));
        EXPECT_CALL(client.handler, handle_input(mt::ButtonDownEvent(99, 99)));
        EXPECT_CALL(client.handler, handle_input(mt::ButtonUpEvent(99, 99)))
            .WillOnce(mt::WakeUp(&client.all_events_received));
    }

    client.start();

    // First we will move the cursor in to the input region on the left side of
    // the window. We should see a click here.
    fake_event_hub()->synthesize_event(mis::a_motion_event().with_movement(1, 1));
    fake_event_hub()->synthesize_event(
        mis::a_button_down_event()
            .of_button(BTN_LEFT)
            .with_action(mis::EventAction::Down));
    fake_event_hub()->synthesize_event(mis::a_button_up_event().of_button(BTN_LEFT));
    // Now in to the dead zone in the center of the window. We should not see
    // a click here.
    fake_event_hub()->synthesize_event(mis::a_motion_event().with_movement(49, 49));
    fake_event_hub()->synthesize_event(
        mis::a_button_down_event()
            .of_button(BTN_LEFT)
            .with_action(mis::EventAction::Down));
    fake_event_hub()->synthesize_event(mis::a_button_up_event().of_button(BTN_LEFT));
    // Now in to the right edge of the window, in the right input region.
    // Again we should see a click.
    fake_event_hub()->synthesize_event(mis::a_motion_event().with_movement(49, 49));
    fake_event_hub()->synthesize_event(
        mis::a_button_down_event()
            .of_button(BTN_LEFT)
            .with_action(mis::EventAction::Down));
    fake_event_hub()->synthesize_event(mis::a_button_up_event().of_button(BTN_LEFT));
}

TEST_F(TestClientInput, scene_obscure_motion_events_by_stacking)
{
    using namespace testing;

    auto smaller_geometry = screen_geometry;
    smaller_geometry.size.width =
        geom::Width{screen_geometry.size.width.as_uint32_t() / 2};

    test_server_config().client_geometries[test_client_name_1] = screen_geometry;
    test_server_config().client_geometries[test_client_name_2] = smaller_geometry;
    test_server_config().client_depths[test_client_name_1] = ms::DepthId{0};
    test_server_config().client_depths[test_client_name_2] = ms::DepthId{1};

    InputClient client1{new_connection(), test_client_name_1};
    InputClient client2{new_connection(), test_client_name_2};

    EXPECT_CALL(client1.handler, handle_input(mt::HoverEnterEvent())).Times(AnyNumber());
    EXPECT_CALL(client1.handler, handle_input(mt::HoverExitEvent())).Times(AnyNumber());
    EXPECT_CALL(client1.handler, handle_input(mt::MovementEvent())).Times(AnyNumber());
    {
        // We should only see one button event sequence.
        InSequence seq;
        EXPECT_CALL(client1.handler, handle_input(mt::ButtonDownEvent(501, 1)));
        EXPECT_CALL(client1.handler, handle_input(mt::ButtonUpEvent(501, 1)))
            .WillOnce(mt::WakeUp(&client1.all_events_received));
    }

    EXPECT_CALL(client2.handler, handle_input(mt::HoverEnterEvent())).Times(AnyNumber());
    EXPECT_CALL(client2.handler, handle_input(mt::HoverExitEvent())).Times(AnyNumber());
    EXPECT_CALL(client2.handler, handle_input(mt::MovementEvent())).Times(AnyNumber());
    {
        // Likewise we should only see one button sequence.
        InSequence seq;
        EXPECT_CALL(client2.handler, handle_input(mt::ButtonDownEvent(1, 1)));
        EXPECT_CALL(client2.handler, handle_input(mt::ButtonUpEvent(1, 1)))
            .WillOnce(mt::WakeUp(&client2.all_events_received));
    }

    client1.start();
    client2.start();

    // First we will move the cursor in to the region where client 2 obscures client 1
    fake_event_hub()->synthesize_event(mis::a_motion_event().with_movement(1, 1));
    fake_event_hub()->synthesize_event(
        mis::a_button_down_event().of_button(BTN_LEFT).with_action(mis::EventAction::Down));
    fake_event_hub()->synthesize_event(mis::a_button_up_event().of_button(BTN_LEFT));
    // Now we move to the unobscured region of client 1
    fake_event_hub()->synthesize_event(mis::a_motion_event().with_movement(500, 0));
    fake_event_hub()->synthesize_event(
        mis::a_button_down_event().of_button(BTN_LEFT).with_action(mis::EventAction::Down));
    fake_event_hub()->synthesize_event(mis::a_button_up_event().of_button(BTN_LEFT));
}

TEST_F(TestClientInput, hidden_clients_do_not_receive_pointer_events)
{
    using namespace testing;

    mt::WaitCondition second_client_done;

    test_server_config().client_depths[test_client_name_1] = ms::DepthId{0};
    test_server_config().client_depths[test_client_name_2] = ms::DepthId{1};

    InputClient client1{new_connection(), test_client_name_1};
    InputClient client2{new_connection(), test_client_name_2};

    EXPECT_CALL(client1.handler, handle_input(mt::HoverEnterEvent())).Times(AnyNumber());
    EXPECT_CALL(client1.handler, handle_input(mt::HoverExitEvent())).Times(AnyNumber());
    EXPECT_CALL(client1.handler, handle_input(mt::MotionEventWithPosition(2, 2)))
        .WillOnce(mt::WakeUp(&client1.all_events_received));

    EXPECT_CALL(client2.handler, handle_input(mt::HoverEnterEvent())).Times(AnyNumber());
    EXPECT_CALL(client2.handler, handle_input(mt::HoverExitEvent())).Times(AnyNumber());
    EXPECT_CALL(client2.handler, handle_input(mt::MotionEventWithPosition(1, 1)))
        .WillOnce(DoAll(mt::WakeUp(&second_client_done),
                        mt::WakeUp(&client2.all_events_received)));

    client1.start();
    client2.start();

    // We send one event and then hide the surface on top before sending the next.
    // So we expect each of the two surfaces to receive one even
    fake_event_hub()->synthesize_event(mis::a_motion_event().with_movement(1,1));
    // We use a fence to ensure we do not hide the client
    // before event dispatch occurs
    second_client_done.wait_for_at_most_seconds(60);

    server_config().the_session_container()->for_each(
        [&](std::shared_ptr<ms::Session> const& session) -> void
        {
            if (session->name() == test_client_name_2)
                session->hide();
        });

    fake_event_hub()->synthesize_event(mis::a_motion_event().with_movement(1,1));
}

TEST_F(TestClientInput, clients_receive_motion_within_coordinate_system_of_window)
{
    using namespace testing;

    int const screen_width = screen_geometry.size.width.as_int();
    int const screen_height = screen_geometry.size.height.as_int();
    int const client_height = screen_height / 2;
    int const client_width = screen_width / 2;

    test_server_config().client_geometries[test_client_name_1] =
        {{screen_width / 2, screen_height / 2}, {client_width, client_height}};

    InputClient client1{new_connection(), test_client_name_1};

    InSequence seq;
    EXPECT_CALL(client1.handler, handle_input(mt::HoverEnterEvent()));
    EXPECT_CALL(client1.handler, handle_input(mt::MotionEventWithPosition(80, 170)))
        .Times(AnyNumber())
        .WillOnce(mt::WakeUp(&client1.all_events_received));

    client1.start();

    server_config().the_session_container()->for_each(
        [&](std::shared_ptr<ms::Session> const& session) -> void
        {
            session->default_surface()->move_to(
                geom::Point{screen_width / 2 - 40, screen_height / 2 - 80});
        });

    fake_event_hub()->synthesize_event(
        mis::a_motion_event().with_movement(screen_width / 2 + 40, screen_height / 2 + 90));
}

// TODO: Consider tests for more input devices with custom mapping (i.e. joysticks...)
TEST_F(TestClientInput, usb_direct_input_devices_work)
{
    using namespace ::testing;

    auto const minimum_touch = mia::FakeEventHub::TouchScreenMinAxisValue;
    auto const maximum_touch = mia::FakeEventHub::TouchScreenMaxAxisValue;
    auto const display_width = screen_geometry.size.width.as_uint32_t();
    auto const display_height = screen_geometry.size.height.as_uint32_t();

    // We place a click 10% in to the touchscreens space in both axis,
    // and a second at 0,0. Thus we expect to see a click at
    // .1*screen_width/height and a second at zero zero.
    static int const abs_touch_x_1 = minimum_touch + (maximum_touch - minimum_touch) * 0.1;
    static int const abs_touch_y_1 = minimum_touch + (maximum_touch - minimum_touch) * 0.1;
    static int const abs_touch_x_2 = 0;
    static int const abs_touch_y_2 = 0;

    static float const expected_scale_x =
        float(display_width) / (maximum_touch - minimum_touch + 1);
    static float const expected_scale_y =
        float(display_height) / (maximum_touch - minimum_touch + 1);

    static float const expected_motion_x_1 = expected_scale_x * abs_touch_x_1;
    static float const expected_motion_y_1 = expected_scale_y * abs_touch_y_1;
    static float const expected_motion_x_2 = expected_scale_x * abs_touch_x_2;
    static float const expected_motion_y_2 = expected_scale_y * abs_touch_y_2;

    test_server_config().client_geometries[test_client_name_1] = screen_geometry;

    InputClient client1{new_connection(), test_client_name_1};

    InSequence seq;
    EXPECT_CALL(client1.handler, handle_input(
        mt::TouchEvent(expected_motion_x_1, expected_motion_y_1)));
    EXPECT_CALL(client1.handler, handle_input(
        mt::MotionEventInDirection(expected_motion_x_1,
                                   expected_motion_y_1,
                                   expected_motion_x_2,
                                   expected_motion_y_2)))
        .WillOnce(mt::WakeUp(&client1.all_events_received));

    client1.start();

    fake_event_hub()->synthesize_event(
        mis::a_touch_event().at_position({abs_touch_x_1, abs_touch_y_1}));
    // Sleep here to trigger more failures (simulate slow machine)
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    fake_event_hub()->synthesize_event(
        mis::a_touch_event().at_position({abs_touch_x_2, abs_touch_y_2}));
}

TEST_F(TestClientInput, send_mir_input_events_through_surface)
{
    InputClient client1{new_connection(), test_client_name_1};

    EXPECT_CALL(client1.handler, handle_input(mt::KeyDownEvent()))
        .WillOnce(mt::WakeUp(&client1.all_events_received));

    client1.start();

    server_config().the_session_container()->for_each(
        [] (std::shared_ptr<ms::Session> const& session) -> void
        {
            MirEvent key_event;
            std::memset(&key_event, 0, sizeof key_event);
            key_event.type = mir_event_type_key;
            key_event.key.action = mir_key_action_down;

            session->default_surface()->consume(key_event);
        });
}
