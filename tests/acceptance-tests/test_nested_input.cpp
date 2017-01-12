/*
 * Copyright © 2015-2016 Canonical Ltd.
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

#include "mir/input/input_device_info.h"
#include "mir/input/input_device_hub.h"
#include "mir/input/device.h"
#include "mir/input/event_filter.h"
#include "mir/input/composite_event_filter.h"

#include "mir_test_framework/fake_input_device.h"
#include "mir_test_framework/stub_server_platform_factory.h"
#include "mir_test_framework/headless_nested_server_runner.h"
#include "mir_test_framework/headless_in_process_server.h"
#include "mir_test_framework/any_surface.h"
#include "mir/test/doubles/nested_mock_egl.h"
#include "mir/test/doubles/mock_input_device_observer.h"
#include "mir/test/event_factory.h"
#include "mir/test/event_matchers.h"
#include "mir/test/fake_shared.h"
#include "mir/test/signal.h"
#include "mir/test/spin_wait.h"

#include "mir_toolkit/mir_client_library.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <linux/input.h>
#include <atomic>

namespace mi = mir::input;
namespace mis = mi::synthesis;

namespace mt = mir::test;
namespace mtd = mt::doubles;
namespace mtf = mir_test_framework;

using namespace ::testing;
using namespace std::chrono_literals;

namespace
{
struct MockEventFilter : public mi::EventFilter
{
    // Work around GMock wanting to know how to construct MirEvent
    MOCK_METHOD1(handle, bool(MirEvent const*));
    bool handle(MirEvent const& ev)
    {
        handle(&ev);
        return true;
    }
};

std::vector<mir::geometry::Rectangle> const display_geometry
{
    {{  0, 0}, {1920, 1080}}
};
    
struct NestedServerWithMockEventFilter : mtf::HeadlessNestedServerRunner
{
    NestedServerWithMockEventFilter(std::string const& connection_string,
                                    std::shared_ptr<MockEventFilter> const& event_filter)
        : mtf::HeadlessNestedServerRunner(connection_string), mock_event_filter{event_filter}
    {
        server.the_composite_event_filter()->append(mock_event_filter);
        start_server();
    }
    NestedServerWithMockEventFilter(std::string const& connection_string)
        : NestedServerWithMockEventFilter(connection_string, std::make_shared<MockEventFilter>())
    {
    }

    ~NestedServerWithMockEventFilter()
    {
        stop_server();
    }

    std::shared_ptr<MockEventFilter> const mock_event_filter;
};

struct NestedInput : public mtf::HeadlessInProcessServer
{

    void SetUp()
    {
        initial_display_layout(display_geometry);
        mtf::HeadlessInProcessServer::SetUp();
    }
    
    mtd::NestedMockEGL mock_egl;

    std::unique_ptr<mtf::FakeInputDevice> fake_keyboard{
        mtf::add_fake_input_device(mi::InputDeviceInfo{"keyboard", "keyboard-uid" , mi::DeviceCapability::keyboard})
    };

    mir::test::Signal all_events_received;
    mir::test::Signal input_device_changes_complete;

    std::shared_ptr<NiceMock<mtd::MockInputDeviceObserver>> mock_observer{
        std::make_shared<NiceMock<mtd::MockInputDeviceObserver>>()};
};

struct NestedInputWithMouse : NestedInput
{
    std::unique_ptr<mtf::FakeInputDevice> fake_mouse{
        mtf::add_fake_input_device(mi::InputDeviceInfo{"mouse", "mouse-uid" , mi::DeviceCapability::pointer})
    };
    MockEventFilter nested_event_filter;
};

struct ExposedSurface
{
public:
    ExposedSurface(std::string const& connect_string)
    {
        // Ensure the nested server posts a frame
        connection = mir_connect_sync(connect_string.c_str(), __PRETTY_FUNCTION__);
        window = mtf::make_any_surface(connection);
        mir_window_set_event_handler(window, handle_event, this);
        mir_buffer_stream_swap_buffers_sync(mir_window_get_buffer_stream(window));
    }

    MOCK_METHOD1(handle_input, void(MirEvent const*));

    void handle_window_event(MirWindowEvent const* event)
    {
        auto const attrib = mir_window_event_get_attribute(event);
        auto const value = mir_window_event_get_attribute_value(event);

        if (mir_window_attrib_visibility == attrib &&
            mir_window_visibility_exposed == value)
            exposed = true;

        if (mir_window_attrib_focus == attrib &&
            mir_window_focus_state_focused == value)
            focused = true;

        if (exposed && focused)
            ready_to_accept_events.raise();
    }

    static void handle_event(MirWindow*, MirEvent const* ev, void* context)
    {
        auto const client = static_cast<ExposedSurface*>(context);
        auto type = mir_event_get_type(ev);
        if (type == mir_event_type_window)
        {
            auto window_event = mir_event_get_window_event(ev);
            client->handle_window_event(window_event);

        }
        if (type == mir_event_type_input)
            client->handle_input(ev);
    }

    static void null_event_handler(MirWindow*, MirEvent const*, void*) {};
    ~ExposedSurface()
    {
        mir_window_set_event_handler(window, null_event_handler, nullptr);
        mir_window_release_sync(window);
        mir_connection_release(connection);
    }
    mir::test::Signal ready_to_accept_events;

protected:
    ExposedSurface(ExposedSurface const&) = delete;
    ExposedSurface& operator=(ExposedSurface const&) = delete;
    
private:
    MirConnection *connection;
    MirWindow *window;
    bool exposed{false};
    bool focused{false};
};

}

TEST_F(NestedInput, nested_event_filter_receives_keyboard_from_host)
{
    MockEventFilter nested_event_filter;
    std::atomic<int> num_key_a_events{0};
    auto const increase_key_a_events = [&num_key_a_events] { ++num_key_a_events; };

    InSequence seq;
    EXPECT_CALL(nested_event_filter, handle(mt::InputDeviceStateEvent()));
    EXPECT_CALL(nested_event_filter, handle(mt::KeyOfScanCode(KEY_A))).
        Times(AtLeast(1)).
        WillRepeatedly(DoAll(InvokeWithoutArgs(increase_key_a_events), Return(true)));

    EXPECT_CALL(nested_event_filter, handle(mt::KeyOfScanCode(KEY_RIGHTSHIFT))).
        Times(2).
        WillOnce(Return(true)).
        WillOnce(DoAll(mt::WakeUp(&all_events_received), Return(true)));

    NestedServerWithMockEventFilter nested_mir{new_connection(), mt::fake_shared(nested_event_filter)};
    ExposedSurface client(nested_mir.new_connection());

    // Because we are testing a nested setup, it's difficult to guarantee
    // that the nested framebuffer window (and consenquently the client window
    // contained in it) is going to be ready (i.e., exposed and focused) to receive
    // events when we send them. We work around this issue by first sending some
    // dummy events and waiting until we receive one of them.
    auto const dummy_events_received = mt::spin_wait_for_condition_or_timeout(
        [&]
        {
            if (num_key_a_events > 0) return true;
            fake_keyboard->emit_event(mis::a_key_down_event().of_scancode(KEY_A));
            fake_keyboard->emit_event(mis::a_key_up_event().of_scancode(KEY_A));
            return false;
        },
        std::chrono::seconds{5});

    EXPECT_TRUE(dummy_events_received);

    fake_keyboard->emit_event(mis::a_key_down_event().of_scancode(KEY_RIGHTSHIFT));
    fake_keyboard->emit_event(mis::a_key_up_event().of_scancode(KEY_RIGHTSHIFT));

    all_events_received.wait_for(std::chrono::seconds{10});
}

TEST_F(NestedInput, nested_input_device_hub_lists_keyboard)
{
    NestedServerWithMockEventFilter nested_mir{new_connection()};
    auto nested_hub = nested_mir.server.the_input_device_hub();

    nested_hub->for_each_input_device(
        [](auto const& dev)
        {
            EXPECT_THAT(dev.name(), StrEq("keyboard"));
            EXPECT_THAT(dev.unique_id(), StrEq("keyboard-uid"));
            EXPECT_THAT(dev.capabilities(), Eq(mi::DeviceCapability::keyboard));
        });
}

TEST_F(NestedInput, on_add_device_observer_gets_device_added_calls_on_existing_devices)
{
    NestedServerWithMockEventFilter nested_mir{new_connection()};
    auto nested_hub = nested_mir.server.the_input_device_hub();

    EXPECT_CALL(*mock_observer, device_added(_)).Times(1);
    EXPECT_CALL(*mock_observer, changes_complete())
        .Times(1)
        .WillOnce(mt::WakeUp(&input_device_changes_complete));

    nested_hub->add_observer(mock_observer);
    input_device_changes_complete.wait_for(10s);
}

TEST_F(NestedInput, device_added_on_host_triggeres_nested_device_observer)
{
    NestedServerWithMockEventFilter nested_mir{new_connection()};
    auto nested_hub = nested_mir.server.the_input_device_hub();

    EXPECT_CALL(*mock_observer, changes_complete()).Times(1)
        .WillOnce(mt::WakeUp(&input_device_changes_complete));
    nested_hub->add_observer(mock_observer);

    input_device_changes_complete.wait_for(10s);
    EXPECT_THAT(input_device_changes_complete.raised(), Eq(true));
    input_device_changes_complete.reset();

    EXPECT_CALL(*mock_observer, changes_complete())
        .WillOnce(mt::WakeUp(&input_device_changes_complete));

    auto mouse = mtf::add_fake_input_device(mi::InputDeviceInfo{"mouse", "mouse-uid" , mi::DeviceCapability::pointer});

    input_device_changes_complete.wait_for(10s);
}

TEST_F(NestedInput, on_input_device_state_nested_server_emits_input_device_state)
{
    mir::test::Signal client_to_host_event_received;
    mir::test::Signal client_to_nested_event_received;
    MockEventFilter nested_event_filter;
    NestedServerWithMockEventFilter nested_mir{new_connection(), mt::fake_shared(nested_event_filter)};
    ExposedSurface client_to_nested_mir(nested_mir.new_connection());

    client_to_nested_mir.ready_to_accept_events.wait_for(1s);
    ExposedSurface client_to_host(new_connection());
    client_to_host.ready_to_accept_events.wait_for(1s);

    EXPECT_CALL(client_to_host, handle_input(mt::KeyOfScanCode(KEY_LEFTALT)))
        .WillOnce(mt::WakeUp(&client_to_host_event_received));
    EXPECT_CALL(nested_event_filter,
                handle(mt::DeviceStateWithPressedKeys(std::vector<uint32_t>({KEY_LEFTALT, KEY_TAB}))))
        .WillOnce(DoAll(mt::WakeUp(&client_to_nested_event_received), Return(true)));

    fake_keyboard->emit_event(mis::a_key_down_event().of_scancode(KEY_LEFTALT));
    fake_keyboard->emit_event(mis::a_key_down_event().of_scancode(KEY_TAB));

    client_to_nested_event_received.wait_for(2s);
    client_to_host_event_received.wait_for(2s);
}

TEST_F(NestedInputWithMouse, mouse_pointer_coordinates_in_nested_server_are_accumulated)
{
    auto const initial_movement_x = 30;
    auto const initial_movement_y = 30;
    auto const second_movement_x = 5;
    auto const second_movement_y = -2;

    mt::Signal devices_ready;
    mt::Signal event_received;
    EXPECT_CALL(nested_event_filter, handle(mt::InputDeviceStateEvent()))
        .WillOnce(DoAll(mt::WakeUp(&devices_ready), Return(true)));

    EXPECT_CALL(nested_event_filter,
                handle(AllOf(mt::PointerEventWithPosition(initial_movement_x, initial_movement_y),
                             mt::PointerEventWithDiff(initial_movement_x, initial_movement_y))))
        .WillOnce(DoAll(mt::WakeUp(&event_received), Return(true)));

    NestedServerWithMockEventFilter nested_mir{new_connection(), mt::fake_shared(nested_event_filter)};
    ExposedSurface client_to_nested_mir(nested_mir.new_connection());
    ASSERT_TRUE(client_to_nested_mir.ready_to_accept_events.wait_for(60s));

    ASSERT_TRUE(devices_ready.wait_for(60s));

    fake_mouse->emit_event(mis::a_pointer_event().with_movement(initial_movement_x, initial_movement_y));
    ASSERT_TRUE(event_received.wait_for(60s));

    event_received.reset();

    EXPECT_CALL(nested_event_filter,
                handle(AllOf(mt::PointerEventWithPosition(initial_movement_x + second_movement_x, initial_movement_y + second_movement_y),
                             mt::PointerEventWithDiff(second_movement_x, second_movement_y))))
        .WillOnce(DoAll(mt::WakeUp(&event_received), Return(true)));

    fake_mouse->emit_event(mis::a_pointer_event().with_movement(second_movement_x, second_movement_y));
    ASSERT_TRUE(event_received.wait_for(60s));
}

TEST_F(NestedInputWithMouse, mouse_pointer_position_is_in_sync_with_host_server)
{
    int const x[] = {30, -10, 10};
    int const y[] = {30, 100, 50};
    int const initial_x = x[0] + x[1];
    int const initial_y = y[0] + y[1];
    int const final_x = initial_x + x[2];
    int const final_y = initial_y + y[2];

    mt::Signal devices_ready;
    mt::Signal event_received;
    fake_mouse->emit_event(mis::a_pointer_event().with_movement(x[0], y[0]));
    fake_mouse->emit_event(mis::a_pointer_event().with_movement(x[1], y[1]));

    EXPECT_CALL(nested_event_filter, handle(
            mt::DeviceStateWithPosition(initial_x, initial_y)))
        .WillOnce(DoAll(mt::WakeUp(&devices_ready), Return(true)));
    EXPECT_CALL(nested_event_filter,
                handle(AllOf(mt::PointerEventWithPosition(final_x, final_y),
                             mt::PointerEventWithDiff(x[2], y[2]))))
        .WillOnce(DoAll(mt::WakeUp(&event_received), Return(true)));

    NestedServerWithMockEventFilter nested_mir{new_connection(), mt::fake_shared(nested_event_filter)};
    ExposedSurface client_to_nested_mir(nested_mir.new_connection());
    ASSERT_TRUE(client_to_nested_mir.ready_to_accept_events.wait_for(60s));
    ASSERT_TRUE(devices_ready.wait_for(60s));

    fake_mouse->emit_event(mis::a_pointer_event().with_movement(x[2], y[2]));
    ASSERT_TRUE(event_received.wait_for(60s));
}
