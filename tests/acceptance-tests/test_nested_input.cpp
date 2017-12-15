/*
 * Copyright © 2015-2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
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
#include "mir/input/mir_input_config.h"
#include "mir/shell/shell_wrapper.h"
#include "mir/scene/session.h"
#include "mir/scene/surface.h"
#include "mir/graphics/event_handler_register.h"

#include "mir_test_framework/fake_input_device.h"
#include "mir_test_framework/stub_server_platform_factory.h"
#include "mir_test_framework/headless_nested_server_runner.h"
#include "mir_test_framework/headless_in_process_server.h"
#include "mir_test_framework/any_surface.h"
#include "mir/test/doubles/nested_mock_egl.h"
#include "mir/test/doubles/mock_input_device_observer.h"
#include "mir/test/doubles/fake_display.h"
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
namespace mg = mir::graphics;
namespace mtd = mt::doubles;
namespace mtf = mir_test_framework;

using namespace ::testing;
using namespace std::chrono_literals;

namespace
{

struct SurfaceTrackingShell : mir::shell::ShellWrapper
{
    SurfaceTrackingShell(
        std::shared_ptr<mir::shell::Shell> wrapped_shell)
        : ShellWrapper{wrapped_shell}, wrapped_shell{wrapped_shell}
    {}

    mir::frontend::SurfaceId create_surface(
        std::shared_ptr<mir::scene::Session> const& session,
        mir::scene::SurfaceCreationParameters const& params,
        std::shared_ptr<mir::frontend::EventSink> const& sink) override
    {
        auto surface_id = wrapped_shell->create_surface(session, params, sink);

        tracked_surfaces[session->name()] =  TrackedSurface{session, surface_id};

        return surface_id;
    }

    std::shared_ptr<mir::scene::Surface> get_surface(std::string const& session_name)
    {
        if (end(tracked_surfaces) == tracked_surfaces.find(session_name))
            return nullptr;
        TrackedSurface & tracked_surface = tracked_surfaces[session_name];
        auto session = tracked_surface.session.lock();
        if (!session)
            return nullptr;
        return session->surface(tracked_surface.surface);
    }

    struct TrackedSurface
    {
        std::weak_ptr<mir::scene::Session> session;
        mir::frontend::SurfaceId surface;
    };
    std::unordered_map<std::string, TrackedSurface> tracked_surfaces;
    std::shared_ptr<mir::shell::Shell> wrapped_shell;
};

struct Display : mtd::FakeDisplay
{
    using mtd::FakeDisplay::FakeDisplay;

    void register_pause_resume_handlers(
                     mg::EventHandlerRegister&,
                     mg::DisplayPauseHandler const& pause_handler,
                     mg::DisplayResumeHandler const& resume_handler) override
    {
        pause_function = pause_handler;
        resume_function = resume_handler;
    }

    void trigger_pause()
    {
        pause_function();
    }
    void trigger_resume()
    {
        resume_function();
    }
    std::function<bool()> pause_function{[](){return false;}};
    std::function<bool()> resume_function{[](){return false;}};
};

size_t get_index_of(MirInputConfig const* config, std::string const& id)
{
    for (size_t i = 0, e = mir_input_config_device_count(config);i != e;++i)
    {
        auto const* device = mir_input_config_get_device(config, i);
        if (mir_input_device_get_unique_id(device) == id)
            return i;
    }
    return mir_input_config_device_count(config);
}

bool contains(MirInputConfig const* config, std::string const& id)
{
    return mir_input_config_device_count(config) != get_index_of(config, id);
}

struct MockEventFilter : public mi::EventFilter
{
    // Work around GMock wanting to know how to construct MirEvent
    MOCK_METHOD1(handle, void(MirEvent const*));
    bool handle(MirEvent const& ev)
    {
        handle(&ev);
        return false;
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
        server.the_composite_event_filter()->prepend(mock_event_filter);
        server.wrap_shell(
            [this](std::shared_ptr<mir::shell::Shell> const& wrapped)
            {
                surfaces = std::make_shared<SurfaceTrackingShell>(wrapped);
                return surfaces;
            });


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

    std::shared_ptr<mir::scene::Surface> get_surface(std::string const& name)
    {
        return surfaces->get_surface(name);
    }
    std::shared_ptr<SurfaceTrackingShell> surfaces;
    std::shared_ptr<MockEventFilter> const mock_event_filter;
};

struct NestedInput : public mtd::NestedMockEGL, mtf::HeadlessInProcessServer
{
    Display display{display_geometry};
    NestedInput()
    {
        preset_display(mt::fake_shared(display));
    }

    ~NestedInput()
    {
        std::this_thread::sleep_for(1s); // TODO - fix race between shutdown and EGL calls
    }

    void SetUp()
    {
        mtf::HeadlessInProcessServer::SetUp();
    }

    void TearDown() override
    {
        mock_observer.reset();
        mtf::HeadlessInProcessServer::TearDown();
    }

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

    void TearDown() override
    {
        Mock::VerifyAndClearExpectations(&nested_event_filter);
        NestedInput::TearDown();
    }
    MockEventFilter nested_event_filter;
};

struct ExposedSurface
{
public:
    using UniqueInputConfig = std::unique_ptr<MirInputConfig , void(*)(MirInputConfig const*)>;
    ExposedSurface(std::string const& connect_string, std::string const& session_name = "ExposedSurface")
    {
        // Ensure the nested server posts a frame
        connection = mir_connect_sync(connect_string.c_str(), session_name.c_str());

        mir_connection_set_input_config_change_callback(
            connection,
            [](MirConnection*, void* context)
            {
                auto* surf = static_cast<ExposedSurface*>(context);
                auto config = surf->get_input_config();
                surf->input_config(config.get());
            },
            this);

        window = mtf::make_any_surface(connection, handle_event, this);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
        mir_buffer_stream_swap_buffers_sync(mir_window_get_buffer_stream(window));
#pragma GCC diagnostic pop
    }

    MOCK_METHOD1(handle_input, void(MirEvent const*));
    MOCK_METHOD1(handle_device_state, void(MirEvent const*));
    MOCK_METHOD0(handle_keymap, void());

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

        test_and_raise();
    }

    void test_and_raise()
    {
        if (exposed && focused && input_device_state_received)
        {
            if (!ready_to_accept_events.raised())
                std::this_thread::sleep_for(1s); // TODO find a way to ensure the nested display has focus
            ready_to_accept_events.raise();
        }
    }

    UniqueInputConfig get_input_config()
    {
        auto config = mir_connection_create_input_config(connection);
        return {config,&mir_input_config_release};
    }

    void on_input_config(std::function<void(MirInputConfig const*)> const& handler)
    {
        input_config = handler;
    }

    void apply_config(MirInputConfig* configuration)
    {
        mir_connection_apply_session_input_config(connection, configuration);
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
        if (type == mir_event_type_keymap)
            client->handle_keymap();
        if (type == mir_event_type_input)
            client->handle_input(ev);
        if (type == mir_event_type_input_device_state)
        {
            client->input_device_state_received = true;
            client->test_and_raise();
            client->handle_device_state(ev);
        }

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
    bool input_device_state_received = false;
    std::function<void(MirInputConfig const* confg)> input_config{[](MirInputConfig const*){}};
};

}

TEST_F(NestedInput, nested_event_filter_receives_keyboard_from_host)
{
    MockEventFilter nested_event_filter;
    std::atomic<int> num_key_a_events{0};
    auto const increase_key_a_events = [&num_key_a_events] { ++num_key_a_events; };
    mt::Signal devices_ready;

    InSequence seq;
    EXPECT_CALL(nested_event_filter, handle(mt::InputDeviceStateEvent()))
        .WillOnce(mt::WakeUp(&devices_ready));
    EXPECT_CALL(nested_event_filter, handle(mt::KeyOfScanCode(KEY_A))).
        Times(AtLeast(1)).
        WillRepeatedly(InvokeWithoutArgs(increase_key_a_events));

    EXPECT_CALL(nested_event_filter, handle(mt::KeyOfScanCode(KEY_RIGHTSHIFT))).
        Times(2).
        WillOnce(mt::WakeUp(&all_events_received));

    NestedServerWithMockEventFilter nested_mir{new_connection(), mt::fake_shared(nested_event_filter)};
    ExposedSurface client(nested_mir.new_connection());

    ASSERT_TRUE(devices_ready.wait_for(60s));

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
        std::chrono::seconds{10});

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

    EXPECT_CALL(*mock_observer, device_added(_))
        .WillOnce(mt::WakeUp(&input_device_changes_complete));

    nested_hub->add_observer(mock_observer);
    input_device_changes_complete.wait_for(10s);
    nested_hub->remove_observer(mock_observer);
}

TEST_F(NestedInput, device_added_on_host_triggeres_nested_device_observer)
{
    NestedServerWithMockEventFilter nested_mir{new_connection()};
    auto nested_hub = nested_mir.server.the_input_device_hub();

    // wait until we see the keyboard from the fixture:
    EXPECT_CALL(*mock_observer, device_added(_)).Times(1)
        .WillOnce(mt::WakeUp(&input_device_changes_complete));
    nested_hub->add_observer(mock_observer);

    input_device_changes_complete.wait_for(10s);
    EXPECT_THAT(input_device_changes_complete.raised(), Eq(true));
    input_device_changes_complete.reset();
    ::testing::Mock::VerifyAndClearExpectations(&mock_observer);

    // wait for the fake mouse
    EXPECT_CALL(*mock_observer, device_added(_)).Times(1)
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
        .WillOnce(mt::WakeUp(&client_to_nested_event_received));

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
        .WillOnce(mt::WakeUp(&devices_ready));

    EXPECT_CALL(nested_event_filter,
                handle(AllOf(mt::PointerEventWithPosition(initial_movement_x, initial_movement_y),
                             mt::PointerEventWithDiff(initial_movement_x, initial_movement_y))))
        .WillOnce(mt::WakeUp(&event_received));

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
        .WillOnce(mt::WakeUp(&event_received));

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
        .WillOnce(mt::WakeUp(&devices_ready));
    EXPECT_CALL(nested_event_filter,
                handle(AllOf(mt::PointerEventWithPosition(final_x, final_y),
                             mt::PointerEventWithDiff(x[2], y[2]))))
        .WillOnce(mt::WakeUp(&event_received));

    NestedServerWithMockEventFilter nested_mir{new_connection(), mt::fake_shared(nested_event_filter)};
    ExposedSurface client_to_nested_mir(nested_mir.new_connection());
    ASSERT_TRUE(client_to_nested_mir.ready_to_accept_events.wait_for(60s));
    ASSERT_TRUE(devices_ready.wait_for(60s));

    fake_mouse->emit_event(mis::a_pointer_event().with_movement(x[2], y[2]));
    ASSERT_TRUE(event_received.wait_for(60s));
}

TEST_F(NestedInput, nested_clients_can_change_host_device_configurations)
{
    auto const acceleration_bias = 0.9f;
    std::string const uid{"mouse-uid"};

    mt::Signal fake_device_received;
    NestedServerWithMockEventFilter nested_mir{new_connection()};
    ExposedSurface client_to_nested(nested_mir.new_connection());
    client_to_nested.on_input_config(
        [&](MirInputConfig const* config)
        {
            if (contains(config, uid))
                fake_device_received.raise();
        });
    client_to_nested.ready_to_accept_events.wait_for(4s);

    std::unique_ptr<mtf::FakeInputDevice> fake_mouse{
        mtf::add_fake_input_device(mi::InputDeviceInfo{"mouse", uid, mi::DeviceCapability::pointer})
    };

    ASSERT_TRUE(fake_device_received.wait_for(4s));

    auto device_config = client_to_nested.get_input_config();
    auto device = mir_input_config_get_mutable_device(device_config.get(), get_index_of(device_config.get(), uid));
    auto ptr_conf = mir_input_device_get_mutable_pointer_config(device);

    mir_pointer_config_set_acceleration_bias(ptr_conf, acceleration_bias);
    fake_device_received.reset();
    client_to_nested.apply_config(device_config.get());

    EXPECT_TRUE(fake_device_received.wait_for(2s));

    device_config = client_to_nested.get_input_config();
    device = mir_input_config_get_mutable_device(device_config.get(), get_index_of(device_config.get(), uid));
    ptr_conf = mir_input_device_get_mutable_pointer_config(device);

    EXPECT_THAT(mir_pointer_config_get_acceleration_bias(ptr_conf), acceleration_bias);
}

TEST_F(NestedInput, pressed_keys_on_vt_switch_are_forgotten)
{
    NiceMock<MockEventFilter> nested_event_filter;

    mt::Signal devices_ready;
    ON_CALL(nested_event_filter, handle(mt::InputDeviceStateEvent()))
        .WillByDefault(mt::WakeUp(&devices_ready));

    NestedServerWithMockEventFilter nested_mir{new_connection(), mt::fake_shared(nested_event_filter)};
    ExposedSurface client_to_nested(nested_mir.new_connection(), "with_keymap");
    ASSERT_TRUE(devices_ready.wait_for(10s));
    ASSERT_TRUE(client_to_nested.ready_to_accept_events.wait_for(10s));

    {
        mt::Signal keymap_received;

        EXPECT_CALL(client_to_nested, handle_keymap()).WillOnce(mt::WakeUp(&keymap_received));

        nested_mir.get_surface("with_keymap")->set_keymap(MirInputDeviceId{0}, "pc105", "de", "", "");

        EXPECT_TRUE(keymap_received.wait_for(10s));
    }

    {
        mt::Signal initial_keys_received;

        EXPECT_CALL(client_to_nested, handle_input(mt::KeyOfScanCode(KEY_RIGHTALT)));
        EXPECT_CALL(client_to_nested, handle_input(mt::KeyOfScanCode(KEY_RIGHTCTRL)))
            .WillOnce(mt::WakeUp(&initial_keys_received));

        fake_keyboard->emit_event(mis::a_key_down_event().of_scancode(KEY_RIGHTALT));
        fake_keyboard->emit_event(mis::a_key_down_event().of_scancode(KEY_RIGHTCTRL));

        EXPECT_TRUE(initial_keys_received.wait_for(10s));
    }

    display.trigger_pause();
    display.trigger_resume();

    // The expectations above are abused to set up the test conditions,
    // clearing them gives clearer errors from the actual test that follows.
    Mock::VerifyAndClearExpectations(&client_to_nested);

    mt::Signal keys_without_modifier_received;

    EXPECT_CALL(client_to_nested,
                handle_input(AllOf(mt::KeyOfScanCode(KEY_A), mt::KeyWithModifiers(mir_input_event_modifier_none))))
        .WillOnce(mt::WakeUp(&keys_without_modifier_received));

    fake_keyboard->emit_event(mis::a_key_down_event().of_scancode(KEY_A));
    EXPECT_TRUE(keys_without_modifier_received.wait_for(10s));
}
