/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#define MIR_INCLUDE_DEPRECATED_EVENT_HEADER

#include "mir/input/input_device_info.h"

#include "mir_test_framework/headless_in_process_server.h"
#include "mir_test_framework/fake_input_device.h"
#include "mir_test_framework/placement_applying_shell.h"
#include "mir_test_framework/stub_server_platform_factory.h"
#include "mir_test/wait_condition.h"
#include "mir_test/spin_wait.h"
#include "mir_test/event_matchers.h"
#include "mir_test/event_factory.h"

#include "mir_toolkit/mir_client_library.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <linux/input.h>

#include <condition_variable>
#include <chrono>
#include <mutex>

namespace mi = mir::input;
namespace mt = mir::test;
namespace mis = mir::input::synthesis;
namespace mtf = mir_test_framework;
namespace geom = mir::geometry;

namespace
{

struct TestClientInputNew : mtf::HeadlessInProcessServer
{
    struct Client
    {
        static const int surface_width = 640;
        static const int surface_height = 480;
        MirSurface* surface{nullptr};

        MOCK_METHOD1(handle_input, void(MirEvent const*));

        Client(char const* con, std::string name)
        {
            connection = mir_connect_sync(con, name.c_str());

            if (!mir_connection_is_valid(connection))
            {
                BOOST_THROW_EXCEPTION(
                    std::runtime_error{std::string{"Failed to connect to test server: "} +
                    mir_connection_get_error_message(connection)});
            }
            auto spec = mir_connection_create_spec_for_normal_surface(connection, surface_width,
                                                                      surface_height, mir_pixel_format_abgr_8888);
            mir_surface_spec_set_name(spec, name.c_str());
            surface = mir_surface_create_sync(spec);
            mir_surface_spec_release(spec);
            if (!mir_surface_is_valid(surface))
                BOOST_THROW_EXCEPTION(std::runtime_error{std::string{"Failed creating a surface: "}+
                    mir_surface_get_error_message(surface)});

            mir_surface_set_event_handler(surface, handle_event, this);
            mir_buffer_stream_swap_buffers_sync(
                mir_surface_get_buffer_stream(surface));

            ready_to_accept_events.wait_for_at_most_seconds(1);
            if (!ready_to_accept_events.woken())
                BOOST_THROW_EXCEPTION(std::runtime_error("Timeout waiting for surface to become focused and exposed"));
        }

        static void handle_event(MirSurface*, MirEvent const* ev, void* context)
        {
            PrintTo(ev, &std::cout);
            std::cout << std::endl;
            auto const client = static_cast<Client*>(context);
            auto type = mir_event_get_type(ev);
            if (type == mir_event_type_surface)
            {
                auto surface_event = mir_event_get_surface_event(ev);

                if (mir_surface_attrib_focus == mir_surface_event_get_attribute(surface_event) &&
                    1 == mir_surface_event_get_attribute_value(surface_event))
                    client->ready_to_accept_events.wake_up_everyone();
            }
            if (type == mir_event_type_input)
                client->handle_input(ev);
        }
        ~Client()
        {
            mir_surface_release_sync(surface);
            mir_connection_release(connection);
        }
        MirConnection * connection;
        mir::test::WaitCondition ready_to_accept_events;
    };

    void SetUp() override
    {
        initial_display_layout(std::vector<geom::Rectangle>({geom::Rectangle({0,0},{1000,800})}));

        server.wrap_shell(
            [this](std::shared_ptr<mir::shell::Shell> const& wrapped)
            {
                return std::make_shared<mtf::PlacementApplyingShell>(
                    wrapped,
                    input_regions, positions, depths);
            });

        HeadlessInProcessServer::SetUp();

        positions[first] = geom::Rectangle{{0,0}, {Client::surface_width, Client::surface_height}};
        first_client = std::make_unique<Client>(new_connection().c_str(), first);
    }

    void TearDown() override
    {
        first_client.reset();
        second_client.reset();
        HeadlessInProcessServer::TearDown();
    }

    std::unique_ptr<mtf::FakeInputDevice> fake_keyboard{
        mtf::add_fake_input_device(mi::InputDeviceInfo{ 0, "keyboard", "keyboard-uid" , mi::DeviceCapability::keyboard})
        };
    std::unique_ptr<mtf::FakeInputDevice> fake_mouse{
        mtf::add_fake_input_device(mi::InputDeviceInfo{ 0, "mouse", "mouse-uid" , mi::DeviceCapability::pointer})
        };
 
    mir::test::WaitCondition all_events_received;
    std::string first{"first"};
    std::string second{"second"};
    mtf::ClientInputRegions input_regions;
    mtf::ClientPositions positions;
    mtf::ClientDepths depths;
    std::unique_ptr<Client> first_client, second_client;
};

const int TestClientInputNew::Client::surface_width;
const int TestClientInputNew::Client::surface_height;
}

TEST_F(TestClientInputNew, clients_receive_keys)
{
    using namespace testing;

    InSequence seq;
    EXPECT_CALL(*first_client, handle_input(AllOf(mt::KeyDownEvent(), mt::KeyOfSymbol(XKB_KEY_Shift_R))));
    EXPECT_CALL(*first_client, handle_input(AllOf(mt::KeyDownEvent(), mt::KeyOfSymbol(XKB_KEY_M))));
    EXPECT_CALL(*first_client, handle_input(AllOf(mt::KeyUpEvent(), mt::KeyOfSymbol(XKB_KEY_M))));
    EXPECT_CALL(*first_client, handle_input(AllOf(mt::KeyUpEvent(), mt::KeyOfSymbol(XKB_KEY_Shift_R))));
    EXPECT_CALL(*first_client, handle_input(AllOf(mt::KeyDownEvent(), mt::KeyOfSymbol(XKB_KEY_i))));
    EXPECT_CALL(*first_client, handle_input(AllOf(mt::KeyUpEvent(), mt::KeyOfSymbol(XKB_KEY_i))));
    EXPECT_CALL(*first_client, handle_input(AllOf(mt::KeyDownEvent(), mt::KeyOfSymbol(XKB_KEY_r))));
    EXPECT_CALL(*first_client, handle_input(AllOf(mt::KeyUpEvent(), mt::KeyOfSymbol(XKB_KEY_r)))).WillOnce(
        mt::WakeUp(&all_events_received));

    fake_keyboard->emit_event(mis::a_key_down_event().of_scancode(KEY_RIGHTSHIFT));
    fake_keyboard->emit_event(mis::a_key_down_event().of_scancode(KEY_M));
    fake_keyboard->emit_event(mis::a_key_up_event().of_scancode(KEY_M));
    fake_keyboard->emit_event(mis::a_key_up_event().of_scancode(KEY_RIGHTSHIFT));
    fake_keyboard->emit_event(mis::a_key_down_event().of_scancode(KEY_I));
    fake_keyboard->emit_event(mis::a_key_up_event().of_scancode(KEY_I));
    fake_keyboard->emit_event(mis::a_key_down_event().of_scancode(KEY_R));
    fake_keyboard->emit_event(mis::a_key_up_event().of_scancode(KEY_R));

    all_events_received.wait_for_at_most_seconds(10);
}

TEST_F(TestClientInputNew, clients_receive_us_english_mapped_keys)
{
    using namespace testing;

    InSequence seq;
    EXPECT_CALL(*first_client, handle_input(AllOf(mt::KeyDownEvent(), mt::KeyOfSymbol(XKB_KEY_Shift_L))));
    EXPECT_CALL(*first_client, handle_input(AllOf(mt::KeyDownEvent(), mt::KeyOfSymbol(XKB_KEY_dollar))))
        .WillOnce(mt::WakeUp(&all_events_received));

    fake_keyboard->emit_event(
        mis::a_key_down_event().of_scancode(KEY_LEFTSHIFT));
    fake_keyboard->emit_event(
        mis::a_key_down_event().of_scancode(KEY_4));
    all_events_received.wait_for_at_most_seconds(10);
}

TEST_F(TestClientInputNew, clients_receive_pointer_inside_window_and_crossing_events)
{
    using namespace testing;

    InSequence seq;

    positions[first] = geom::Rectangle{{0,0}, {Client::surface_width, Client::surface_height}};

    // We should see the cursor enter
    EXPECT_CALL(*first_client, handle_input(mt::PointerEnterEvent()));
    EXPECT_CALL(*first_client, handle_input(mt::PointerEventWithPosition(
                Client::surface_width - 1,
                Client::surface_height - 1)));
    EXPECT_CALL(*first_client, handle_input(mt::PointerLeaveEvent()))
        .WillOnce(mt::WakeUp(&all_events_received));
    // But we should not receive an event for the second movement outside of our surface!

    fake_mouse->emit_event(
        mis::a_pointer_event().with_movement(Client::surface_width - 1, Client::surface_height - 1));
    fake_mouse->emit_event(
        mis::a_pointer_event().with_movement(2, 2));

    all_events_received.wait_for_at_most_seconds(120);
}

TEST_F(TestClientInputNew, clients_receive_button_events_inside_window)
{
    using namespace testing;

    // The cursor starts at (0, 0).
    EXPECT_CALL(*first_client, handle_input(mt::ButtonDownEvent(0, 0)))
        .WillOnce(mt::WakeUp(&all_events_received));

    fake_mouse->emit_event(
        mis::a_button_down_event()
            .of_button(BTN_LEFT)
            .with_action(mis::EventAction::Down));

    all_events_received.wait_for_at_most_seconds(10);
}

#if 0
TEST_F(TestClientInput, multiple_clients_receive_pointer_inside_windows)
{
    using namespace testing;

    second_client = std::make_unique<Client>(new_connection().c_str());

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
        EXPECT_CALL(*first_client, handle_input(mt::PointerEnterEvent()));
        EXPECT_CALL(*first_client,
                    handle_input(
                        mt::PointerEventWithPosition(client_width - 1, client_height - 1)));
        EXPECT_CALL(*first_client, handle_input(mt::PointerLeaveEvent()));
    }

    {
        InSequence seq;
        EXPECT_CALL(*second_client, handle_input(mt::PointerEnterEvent()));
        EXPECT_CALL(*second_client,
                    handle_input(
                        mt::PointerEventWithPosition(client_width - 1, client_height - 1)))
            .WillOnce(mt::WakeUp(all_events_received));
    }

    // In the bounds of the first surface
    fake_mouse->emit_event(
        mis::a_pointer_event().with_movement(screen_width / 2 - 1, screen_height / 2 - 1));
    // In the bounds of the second surface
    fake_mouse->emit_event(
        mis::a_pointer_event().with_movement(screen_width / 2, screen_height / 2));

    all_events_received.wait_for_at_most_seconds(2);
}
#endif

