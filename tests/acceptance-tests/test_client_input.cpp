/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#include "mir/input/input_device_info.h"
#include "mir/input/event_filter.h"
#include "mir/input/keymap.h"
#include "mir/input/composite_event_filter.h"
#include "mir/scene/session.h"
#include "mir/scene/surface.h"
#include "mir/input/mir_touchpad_config.h"
#include "mir/input/mir_input_config.h"
#include "mir/input/input_device.h"
#include "mir/input/touchscreen_settings.h"

#include "mir_test_framework/headless_in_process_server.h"
#include "mir_test_framework/input_device_faker.h"
#include "mir_test_framework/fake_input_device.h"
#include "mir_test_framework/placement_applying_shell.h"
#include "mir_test_framework/stub_server_platform_factory.h"
#include "mir_test_framework/temporary_environment_value.h"
#include "mir/test/signal_actions.h"
#include "mir/test/spin_wait.h"
#include "mir/test/event_matchers.h"
#include "mir/test/event_factory.h"
#include "mir/test/fake_shared.h"
#include "mir/test/doubles/stub_session_authorizer.h"

#include "mir/input/input_device_observer.h"
#include "mir_toolkit/mir_client_library.h"
#include "mir/events/event_builders.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <linux/input.h>

#include <condition_variable>
#include <unordered_map>
#include <chrono>
#include <atomic>
#include <mutex>

namespace mi = mir::input;
namespace mt = mir::test;
namespace ms = mir::scene;
namespace mf = mir::frontend;
namespace mis = mir::input::synthesis;
namespace mtf = mir_test_framework;
namespace mtd = mir::test::doubles;
namespace geom = mir::geometry;

using namespace std::chrono_literals;
using namespace testing;

namespace
{

struct StubAuthorizer : mtd::StubSessionAuthorizer
{
    bool configure_input_is_allowed(mf::SessionCredentials const&) override
    {
        return allow_configure_input;
    }

    bool set_base_input_configuration_is_allowed(mf::SessionCredentials const&) override
    {
        return allow_set_base_input_configuration;
    }

    std::atomic<bool> allow_configure_input{true};
    std::atomic<bool> allow_set_base_input_configuration{true};
};

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


template<typename SetCallback, typename Apply>
void apply_and_wait_for_completion(MirConnection* connection, SetCallback set_callback, Apply apply)
{
    mt::Signal change_complete;
    set_callback(connection,
                 [](MirConnection*, void* context)
                 {
                     static_cast<mt::Signal*>(context)->raise(); 
                 },
                 &change_complete
                );
    apply(connection);
    ASSERT_TRUE(change_complete.wait_for(10s));
}

const int surface_width = 100;
const int surface_height = 100;

void null_event_handler(MirWindow*, MirEvent const*, void*)
{
}

struct SurfaceTrackingShell : mir::shell::ShellWrapper
{
    SurfaceTrackingShell(
        std::shared_ptr<mir::shell::Shell> wrapped_shell)
        : ShellWrapper{wrapped_shell}, wrapped_shell{wrapped_shell}
    {}

    auto create_surface(
        std::shared_ptr<mir::scene::Session> const& session,
        mir::scene::SurfaceCreationParameters const& params,
        std::shared_ptr<mir::scene::SurfaceObserver> const& observer) -> std::shared_ptr<mir::scene::Surface> override
    {
        auto const surface = wrapped_shell->create_surface(session, params, observer);

        tracked_surfaces[session->name()] =  {session, surface};

        return surface;
    }

    std::shared_ptr<mir::scene::Surface> get_surface(std::string const& session_name)
    {
        if (end(tracked_surfaces) == tracked_surfaces.find(session_name))
            return nullptr;
        TrackedSurface & tracked_surface = tracked_surfaces[session_name];
        auto session = tracked_surface.session.lock();
        if (!session)
            return nullptr;
        return tracked_surface.surface;
    }

    struct TrackedSurface
    {
        std::weak_ptr<mir::scene::Session> session;
        std::shared_ptr<mir::scene::Surface> surface;
    };
    std::unordered_map<std::string, TrackedSurface> tracked_surfaces;
    std::shared_ptr<mir::shell::Shell> wrapped_shell;
};

struct Client
{
    MirWindow* window{nullptr};

    MOCK_METHOD1(handle_input, void(MirEvent const*));
    MOCK_METHOD1(handle_keymap, void(MirEvent const*));
    MOCK_METHOD1(handle_input_device_state, void(MirEvent const*));

    Client(std::string const& con, std::string const& name)
    {
        connection = mir_connect_sync(con.c_str(), name.c_str());

        if (!mir_connection_is_valid(connection))
        {
            BOOST_THROW_EXCEPTION(
                std::runtime_error{std::string{"Failed to connect to test server: "} +
                mir_connection_get_error_message(connection)});
        }
        auto spec = mir_create_normal_window_spec(connection, surface_width, surface_height);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
        mir_window_spec_set_pixel_format(spec, mir_pixel_format_abgr_8888);
        mir_window_spec_set_buffer_usage(spec, mir_buffer_usage_software);
#pragma GCC diagnostic pop
        mir_window_spec_set_event_handler(spec, handle_event, this);
        mir_window_spec_set_name(spec, name.c_str());
        window = mir_create_window_sync(spec);
        mir_window_spec_release(spec);
        if (!mir_window_is_valid(window))
            BOOST_THROW_EXCEPTION(std::runtime_error{std::string{"Failed creating a window: "}+
                mir_window_get_error_message(window)});

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
        mir_buffer_stream_swap_buffers_sync(
            mir_window_get_buffer_stream(window));
#pragma GCC diagnostic pop
        ready_to_accept_events.wait_for(4s);
        if (!ready_to_accept_events.raised())
            BOOST_THROW_EXCEPTION(std::runtime_error("Timeout waiting for window to become focused and exposed"));
    }

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
            ready_to_accept_events.raise();
    }

    static void handle_event(MirWindow*, MirEvent const* ev, void* context)
    {
        auto const client = static_cast<Client*>(context);
        std::lock_guard<std::mutex> lock(client->client_status);
        auto type = mir_event_get_type(ev);
        if (type == mir_event_type_window)
        {
            auto window_event = mir_event_get_window_event(ev);
            client->handle_window_event(window_event);
        }
        if (type == mir_event_type_input)
            client->handle_input(ev);
        if (type == mir_event_type_keymap)
            client->handle_keymap(ev);
        if (type == mir_event_type_input_device_state)
        {
            client->input_device_state_received = true;
            client->test_and_raise();
            client->handle_input_device_state(ev);
        }
    }
    ~Client()
    {
        // Remove the event handler to avoid handling spurious events unrelated
        // to the tests (e.g. pointer leave events when the window is destroyed),
        // which can cause test expectations to fail.
        mir_connection_set_input_config_change_callback(connection, [](MirConnection*, void*){}, nullptr);

        mir_window_set_event_handler(window, null_event_handler, nullptr);
        mir_window_release_sync(window);
        mir_connection_release(connection);
    }
    MirConnection * connection;
    mir::test::Signal ready_to_accept_events;
    mir::test::Signal all_events_received;
    bool exposed = false;
    bool focused = false;
    bool input_device_state_received = false;
    std::mutex client_status;
};

struct TestClientInput : mtf::HeadlessInProcessServer, mtf::InputDeviceFaker
{
    void SetUp() override
    {
        initial_display_layout({screen_geometry});

        server.wrap_shell(
            [this](std::shared_ptr<mir::shell::Shell> const& wrapped)
            {
                shell = std::make_shared<mtf::PlacementApplyingShell>(wrapped, input_regions, positions);
                surfaces = std::make_shared<SurfaceTrackingShell>(shell);
                return surfaces;
            });
        server.override_the_session_authorizer([this] { return mt::fake_shared(stub_authorizer); });

        HeadlessInProcessServer::SetUp();

        positions[first] = geom::Rectangle{{0,0}, {surface_width, surface_height}};
        wait_for_input_devices_added_to(server);
    }

    std::shared_ptr<mir::scene::Surface> get_surface(std::string const& name)
    {
        return surfaces->get_surface(name);
    }

    std::shared_ptr<mtf::PlacementApplyingShell> shell;
    std::shared_ptr<SurfaceTrackingShell> surfaces;
    std::string const keyboard_name = "keyboard";
    std::string const keyboard_unique_id = "keyboard-uid";
    std::string const mouse_name = "mouse";
    std::string const mouse_unique_id = "mouse-uid";
    std::string const touchscreen_name = "touchscreen";
    std::string const touchscreen_unique_id = "touchscreen-uid";
    std::unique_ptr<mtf::FakeInputDevice> fake_keyboard{add_fake_input_device(mi::InputDeviceInfo{
        keyboard_name, keyboard_unique_id, mi::DeviceCapability::keyboard | mi::DeviceCapability::alpha_numeric})};
    std::unique_ptr<mtf::FakeInputDevice> fake_mouse{
        add_fake_input_device(mi::InputDeviceInfo{mouse_name, mouse_unique_id, mi::DeviceCapability::pointer})};
    std::unique_ptr<mtf::FakeInputDevice> fake_touch_screen{add_fake_input_device(
        mi::InputDeviceInfo{touchscreen_name, touchscreen_unique_id,
                            mi::DeviceCapability::touchscreen | mi::DeviceCapability::multitouch})};

    std::string first{"first"};
    std::string second{"second"};
    mtf::ClientInputRegions input_regions;
    mtf::ClientPositions positions;
    StubAuthorizer stub_authorizer;
    geom::Rectangle screen_geometry{{0,0}, {1000,800}};
    std::shared_ptr<MockEventFilter> mock_event_filter = std::make_shared<MockEventFilter>();

    MirInputDevice const* get_device_with_capabilities(MirInputConfig const* config, MirInputDeviceCapabilities caps)
    {
        for (size_t i = 0; i != mir_input_config_device_count(config); ++i)
        {
            auto dev = mir_input_config_get_device(config, i);
            if (caps == mir_input_device_get_capabilities(dev))
                return dev;
        }
        return nullptr;
    }

    MirInputDevice* get_mutable_device_with_capabilities(MirInputConfig* config, MirInputDeviceCapabilities caps)
    {
        for (size_t i = 0; i != mir_input_config_device_count(config); ++i)
        {
            auto dev = mir_input_config_get_mutable_device(config, i);
            if (caps == mir_input_device_get_capabilities(dev))
                return dev;
        }
        return nullptr;
    }
};

struct TestClientInputWithTwoScreens : TestClientInput
{
    geom::Rectangle second_screen{{1000,0}, {200,400}};

    int const width{second_screen.size.width.as_int()};
    int const height{second_screen.size.height.as_int()};

    void SetUp() override
    {
        initial_display_layout({screen_geometry, second_screen});

        server.wrap_shell(
            [this](std::shared_ptr<mir::shell::Shell> const& wrapped)
            {
                shell = std::make_shared<mtf::PlacementApplyingShell>(wrapped, input_regions, positions);
                return shell;
            });

        HeadlessInProcessServer::SetUp();

        // make surface span over both outputs to test coordinates mapped into second ouput:
        positions[first] =
            geom::Rectangle{{0, 0}, {second_screen.bottom_right().x.as_int(), second_screen.bottom_right().y.as_int()}};
    }
};

}

TEST_F(TestClientInput, clients_receive_keys)
{
    Client first_client(new_connection(), first);

    InSequence seq;
    EXPECT_CALL(first_client, handle_input(AllOf(mt::KeyDownEvent(), mt::KeyOfSymbol(XKB_KEY_Shift_R))));
    EXPECT_CALL(first_client, handle_input(AllOf(mt::KeyDownEvent(), mt::KeyOfSymbol(XKB_KEY_M))));
    EXPECT_CALL(first_client, handle_input(AllOf(mt::KeyUpEvent(), mt::KeyOfSymbol(XKB_KEY_M))));
    EXPECT_CALL(first_client, handle_input(AllOf(mt::KeyUpEvent(), mt::KeyOfSymbol(XKB_KEY_Shift_R))));
    EXPECT_CALL(first_client, handle_input(AllOf(mt::KeyDownEvent(), mt::KeyOfSymbol(XKB_KEY_i))));
    EXPECT_CALL(first_client, handle_input(AllOf(mt::KeyUpEvent(), mt::KeyOfSymbol(XKB_KEY_i))));
    EXPECT_CALL(first_client, handle_input(AllOf(mt::KeyDownEvent(), mt::KeyOfSymbol(XKB_KEY_r))));
    EXPECT_CALL(first_client, handle_input(AllOf(mt::KeyUpEvent(), mt::KeyOfSymbol(XKB_KEY_r)))).WillOnce(
        mt::WakeUp(&first_client.all_events_received));

    fake_keyboard->emit_event(mis::a_key_down_event().of_scancode(KEY_RIGHTSHIFT));
    fake_keyboard->emit_event(mis::a_key_down_event().of_scancode(KEY_M));
    fake_keyboard->emit_event(mis::a_key_up_event().of_scancode(KEY_M));
    fake_keyboard->emit_event(mis::a_key_up_event().of_scancode(KEY_RIGHTSHIFT));
    fake_keyboard->emit_event(mis::a_key_down_event().of_scancode(KEY_I));
    fake_keyboard->emit_event(mis::a_key_up_event().of_scancode(KEY_I));
    fake_keyboard->emit_event(mis::a_key_down_event().of_scancode(KEY_R));
    fake_keyboard->emit_event(mis::a_key_up_event().of_scancode(KEY_R));

    first_client.all_events_received.wait_for(10s);
}

TEST_F(TestClientInput, clients_receive_us_english_mapped_keys)
{
    Client first_client(new_connection(), first);

    InSequence seq;
    EXPECT_CALL(first_client, handle_input(AllOf(mt::KeyDownEvent(), mt::KeyOfSymbol(XKB_KEY_Shift_L))));
    EXPECT_CALL(first_client, handle_input(AllOf(mt::KeyDownEvent(), mt::KeyOfSymbol(XKB_KEY_dollar))))
        .WillOnce(mt::WakeUp(&first_client.all_events_received));

    fake_keyboard->emit_event(mis::a_key_down_event().of_scancode(KEY_LEFTSHIFT));
    fake_keyboard->emit_event(mis::a_key_down_event().of_scancode(KEY_4));
    first_client.all_events_received.wait_for(10s);
}

TEST_F(TestClientInput, clients_receive_pointer_inside_window_and_crossing_events)
{
    positions[first] = geom::Rectangle{{0,0}, {surface_width, surface_height}};
    Client first_client(new_connection(), first);

    // We should see the cursor enter
    InSequence seq;
    EXPECT_CALL(first_client, handle_input(mt::PointerEnterEventWithPosition(surface_width - 1, surface_height - 1)));
    EXPECT_CALL(first_client, handle_input(mt::PointerLeaveEvent()))
        .WillOnce(mt::WakeUp(&first_client.all_events_received));
    // But we should not receive an event for the second movement outside of our window!

    fake_mouse->emit_event(mis::a_pointer_event().with_movement(surface_width - 1, surface_height - 1));
    fake_mouse->emit_event(mis::a_pointer_event().with_movement(2, 2));

    first_client.all_events_received.wait_for(120s);
}

TEST_F(TestClientInput, clients_receive_relative_pointer_events)
{
    positions[first] = geom::Rectangle{{0,0}, {surface_width, surface_height}};
    Client first_client(new_connection(), first);

    InSequence seq;
    EXPECT_CALL(first_client, handle_input(AllOf(mt::PointerEnterEventWithPosition(1, 1),
                                                 mt::PointerEnterEventWithDiff(1, 1))));
    EXPECT_CALL(first_client, handle_input(AllOf(mt::PointerEventWithPosition(2, 2), mt::PointerEventWithDiff(1, 1))));
    EXPECT_CALL(first_client, handle_input(AllOf(mt::PointerEventWithPosition(3, 3), mt::PointerEventWithDiff(1, 1))));
    EXPECT_CALL(first_client, handle_input(AllOf(mt::PointerEventWithPosition(2, 2), mt::PointerEventWithDiff(-1, -1))));
    EXPECT_CALL(first_client, handle_input(AllOf(mt::PointerEventWithPosition(1, 1), mt::PointerEventWithDiff(-1, -1))));
    // Ensure we continue to receive relative moement even when absolute movement is constrained.
    EXPECT_CALL(first_client, handle_input(AllOf(mt::PointerEventWithPosition(0, 0), mt::PointerEventWithDiff(-1, -1))));
    EXPECT_CALL(first_client, handle_input(AllOf(mt::PointerEventWithPosition(0, 0), mt::PointerEventWithDiff(-1, -1))));
    EXPECT_CALL(first_client, handle_input(AllOf(mt::PointerEventWithPosition(0, 0), mt::PointerEventWithDiff(-1, -1))))
        .WillOnce(mt::WakeUp(&first_client.all_events_received));

    fake_mouse->emit_event(mis::a_pointer_event().with_movement(1, 1));
    fake_mouse->emit_event(mis::a_pointer_event().with_movement(1, 1));
    fake_mouse->emit_event(mis::a_pointer_event().with_movement(1, 1));
    fake_mouse->emit_event(mis::a_pointer_event().with_movement(-1, -1));
    fake_mouse->emit_event(mis::a_pointer_event().with_movement(-1, -1));
    fake_mouse->emit_event(mis::a_pointer_event().with_movement(-1, -1));
    fake_mouse->emit_event(mis::a_pointer_event().with_movement(-1, -1));
    fake_mouse->emit_event(mis::a_pointer_event().with_movement(-1, -1));

    first_client.all_events_received.wait_for(120s);
}

TEST_F(TestClientInput, clients_receive_button_events_inside_window)
{
    Client first_client(new_connection(), first);

    EXPECT_CALL(first_client, handle_input(mt::PointerEnterEvent()));
    // The cursor starts at (0, 0).
    EXPECT_CALL(first_client, handle_input(mt::ButtonDownEvent(0, 0)))
        .WillOnce(mt::WakeUp(&first_client.all_events_received));

    fake_mouse->emit_event(mis::a_button_down_event().of_button(BTN_LEFT).with_action(mis::EventAction::Down));

    first_client.all_events_received.wait_for(10s);
}

TEST_F(TestClientInput, clients_receive_many_button_events_inside_window)
{
    Client first_client(new_connection(), first);
    // The cursor starts at (0, 0).

    InSequence seq;
    auto expect_buttons = [&](MirPointerButtons b) {
        EXPECT_CALL(first_client, handle_input(mt::ButtonsDown(0, 0, b)));
    };

    MirPointerButtons buttons = mir_pointer_button_primary;
    EXPECT_CALL(first_client, handle_input(mt::PointerEnterEvent()));
    expect_buttons(buttons);
    expect_buttons(buttons |= mir_pointer_button_secondary);
    expect_buttons(buttons |= mir_pointer_button_tertiary);
    expect_buttons(buttons |= mir_pointer_button_forward);
    expect_buttons(buttons |= mir_pointer_button_back);
    expect_buttons(buttons &= ~mir_pointer_button_back);
    expect_buttons(buttons &= ~mir_pointer_button_forward);
    expect_buttons(buttons &= ~mir_pointer_button_tertiary);
    expect_buttons(buttons &= ~mir_pointer_button_secondary);
    EXPECT_CALL(first_client, handle_input(mt::ButtonsDown(0, 0, 0))).WillOnce(
    mt::WakeUp(&first_client.all_events_received));

    auto press_button = [&](int button) {
        fake_mouse->emit_event(mis::a_button_down_event().of_button(button).with_action(mis::EventAction::Down));
    };
    auto release_button = [&](int button) {
        fake_mouse->emit_event(mis::a_button_up_event().of_button(button).with_action(mis::EventAction::Up));
    };
    press_button(BTN_LEFT);
    press_button(BTN_RIGHT);
    press_button(BTN_MIDDLE);
    press_button(BTN_FORWARD);
    press_button(BTN_BACK);
    release_button(BTN_BACK);
    release_button(BTN_FORWARD);
    release_button(BTN_MIDDLE);
    release_button(BTN_RIGHT);
    release_button(BTN_LEFT);

    first_client.all_events_received.wait_for(10s);
}

TEST_F(TestClientInput, multiple_clients_receive_pointer_inside_windows)
{
    int const screen_width = screen_geometry.size.width.as_int();
    int const screen_height = screen_geometry.size.height.as_int();
    int const client_height = screen_height / 2;
    int const client_width = screen_width / 2;

    positions[first] = {{0, 0}, {client_width, client_height}};
    positions[second] = {{client_width, client_height}, {client_width, client_height}};

    Client first_client(new_connection(), first);
    Client second_client(new_connection(), second);

    {
        InSequence seq;
        EXPECT_CALL(first_client, handle_input(mt::PointerEnterEventWithPosition(client_width - 1, client_height - 1)));
        EXPECT_CALL(first_client, handle_input(mt::PointerLeaveEvent()))
            .WillOnce(mt::WakeUp(&first_client.all_events_received));
    }

    {
        InSequence seq;
        EXPECT_CALL(second_client, handle_input(mt::PointerEnterEventWithPosition(client_width - 1, client_height - 1)))
            .WillOnce(mt::WakeUp(&second_client.all_events_received));
    }

    // In the bounds of the first window
    fake_mouse->emit_event(mis::a_pointer_event().with_movement(client_width - 1, client_height - 1));
    // In the bounds of the second window
    fake_mouse->emit_event(mis::a_pointer_event().with_movement(client_width, client_height));

    first_client.all_events_received.wait_for(2s);
    second_client.all_events_received.wait_for(2s);
}

TEST_F(TestClientInput, clients_do_not_receive_pointer_outside_input_region)
{
    int const client_height = surface_height;
    int const client_width = surface_width;

    input_regions[first] = {{{0, 0}, {client_width - 80, client_height}},
                            {{client_width - 20, 0}, {client_width - 80, client_height}}};

    Client first_client(new_connection(), first);

    EXPECT_CALL(first_client, handle_input(mt::PointerEnterEvent())).Times(AnyNumber());
    EXPECT_CALL(first_client, handle_input(mt::PointerLeaveEvent())).Times(AnyNumber());
    EXPECT_CALL(first_client, handle_input(mt::PointerMovementEvent())).Times(AnyNumber());

    {
        // We should see two of the three button pairs.
        InSequence seq;
        EXPECT_CALL(first_client, handle_input(mt::ButtonDownEvent(1, 1)));
        EXPECT_CALL(first_client, handle_input(mt::ButtonUpEvent(1, 1)));
        EXPECT_CALL(first_client, handle_input(mt::ButtonDownEvent(99, 99)));
        EXPECT_CALL(first_client, handle_input(mt::ButtonUpEvent(99, 99)))
            .WillOnce(mt::WakeUp(&first_client.all_events_received));
    }

    // First we will move the cursor in to the input region on the left side of
    // the window. We should see a click here.
    fake_mouse->emit_event(mis::a_pointer_event().with_movement(1, 1));
    fake_mouse->emit_event(mis::a_button_down_event().of_button(BTN_LEFT).with_action(mis::EventAction::Down));
    fake_mouse->emit_event(mis::a_button_up_event().of_button(BTN_LEFT));

    // Now in to the dead zone in the center of the window. We should not see
    // a click here.
    fake_mouse->emit_event(mis::a_pointer_event().with_movement(49, 49));
    fake_mouse->emit_event(mis::a_button_down_event().of_button(BTN_LEFT).with_action(mis::EventAction::Down));
    fake_mouse->emit_event(mis::a_button_up_event().of_button(BTN_LEFT));

    // Now in to the right edge of the window, in the right input region.
    // Again we should see a click.
    fake_mouse->emit_event(mis::a_pointer_event().with_movement(49, 49));
    fake_mouse->emit_event(mis::a_button_down_event().of_button(BTN_LEFT).with_action(mis::EventAction::Down));
    fake_mouse->emit_event(mis::a_button_up_event().of_button(BTN_LEFT));

    first_client.all_events_received.wait_for(5s);
}

TEST_F(TestClientInput, scene_obscure_motion_events_by_stacking)
{
    auto smaller_geometry = screen_geometry;
    smaller_geometry.size.width =
        geom::Width{screen_geometry.size.width.as_uint32_t() / 2};

    positions[first] = screen_geometry;
    positions[second] = smaller_geometry;

    Client first_client(new_connection(), first);
    Client second_client(new_connection(), second);

    EXPECT_CALL(first_client, handle_input(mt::PointerEnterEvent())).Times(AnyNumber());
    EXPECT_CALL(first_client, handle_input(mt::PointerLeaveEvent())).Times(AnyNumber());
    EXPECT_CALL(first_client, handle_input(mt::PointerMovementEvent())).Times(AnyNumber());
    {
        // We should only see one button event sequence.
        InSequence seq;
        EXPECT_CALL(first_client, handle_input(mt::ButtonDownEvent(501, 1)));
        EXPECT_CALL(first_client, handle_input(mt::ButtonUpEvent(501, 1)))
            .WillOnce(mt::WakeUp(&first_client.all_events_received));
    }

    EXPECT_CALL(second_client, handle_input(mt::PointerEnterEvent())).Times(AnyNumber());
    EXPECT_CALL(second_client, handle_input(mt::PointerLeaveEvent())).Times(AnyNumber());
    EXPECT_CALL(second_client, handle_input(mt::PointerMovementEvent())).Times(AnyNumber());
    {
        // Likewise we should only see one button sequence.
        InSequence seq;
        EXPECT_CALL(second_client, handle_input(mt::ButtonDownEvent(1, 1)));
        EXPECT_CALL(second_client, handle_input(mt::ButtonUpEvent(1, 1)))
            .WillOnce(mt::WakeUp(&second_client.all_events_received));
    }

    // First we will move the cursor in to the region where client 2 obscures client 1
    fake_mouse->emit_event(mis::a_pointer_event().with_movement(1, 1));
    fake_mouse->emit_event(
        mis::a_button_down_event().of_button(BTN_LEFT).with_action(mis::EventAction::Down));
    fake_mouse->emit_event(mis::a_button_up_event().of_button(BTN_LEFT));
    // Now we move to the unobscured region of client 1
    fake_mouse->emit_event(mis::a_pointer_event().with_movement(500, 0));
    fake_mouse->emit_event(mis::a_button_down_event().of_button(BTN_LEFT).with_action(mis::EventAction::Down));
    fake_mouse->emit_event(mis::a_button_up_event().of_button(BTN_LEFT));

    first_client.all_events_received.wait_for(5s);
    second_client.all_events_received.wait_for(5s);
}

TEST_F(TestClientInput, hidden_clients_do_not_receive_pointer_events)
{
    positions[second] = {{0,0}, {surface_width, surface_height}};

    Client first_client(new_connection(), first);
    Client second_client(new_connection(), second);

    EXPECT_CALL(second_client, handle_input(mt::PointerEnterEventWithPosition(1, 1)))
        .WillOnce(mt::WakeUp(&second_client.all_events_received));

    EXPECT_CALL(second_client, handle_input(mt::PointerLeaveEvent())).Times(AnyNumber());

    EXPECT_CALL(first_client, handle_input(mt::PointerEnterEventWithPosition(2, 2)))
        .WillOnce(mt::WakeUp(&first_client.all_events_received));

    // We send one event and then hide the window on top before sending the next.
    // So we expect each of the two surfaces to receive one event
    fake_mouse->emit_event(mis::a_pointer_event().with_movement(1,1));

    second_client.all_events_received.wait_for(2s);

    server.the_shell()->focused_session()->hide();

    fake_mouse->emit_event(mis::a_pointer_event().with_movement(1,1));
    first_client.all_events_received.wait_for(2s);
}

TEST_F(TestClientInput, clients_receive_pointer_within_coordinate_system_of_window)
{
    int const screen_width = screen_geometry.size.width.as_int();
    int const screen_height = screen_geometry.size.height.as_int();
    int const client_height = screen_height / 2;
    int const client_width = screen_width / 2;

    positions[first] = {{screen_width / 2, screen_height / 2}, {client_width, client_height}};

    Client first_client(new_connection(), first);

    InSequence seq;
    EXPECT_CALL(first_client, handle_input(mt::PointerEnterEvent()));
    EXPECT_CALL(first_client, handle_input(mt::PointerEventWithPosition(80, 170)))
        .Times(AnyNumber())
        .WillOnce(mt::WakeUp(&first_client.all_events_received));

    server.the_shell()->focused_surface()->move_to(geom::Point{screen_width / 2 - 40, screen_height / 2 - 80});

    fake_mouse->emit_event(mis::a_pointer_event().with_movement(screen_width / 2 + 40, screen_height / 2 + 90));

    first_client.all_events_received.wait_for(2s);
}

// TODO: Consider tests for more input devices with custom mapping (i.e. joysticks...)
TEST_F(TestClientInput, usb_direct_input_devices_work)
{
    auto const display_width = screen_geometry.size.width.as_int();
    auto const display_height = screen_geometry.size.height.as_int();

    float const expected_motion_x_1 = display_width * 0.1f;
    float const expected_motion_y_1 = display_height * 0.1f;
    float const expected_motion_x_2 = 0;
    float const expected_motion_y_2 = 0;

    positions[first] = screen_geometry;

    Client first_client(new_connection(), first);

    InSequence seq;
    EXPECT_CALL(first_client, handle_input(
        mt::TouchEvent(expected_motion_x_1, expected_motion_y_1)));
    EXPECT_CALL(first_client, handle_input(
        mt::TouchEventInDirection(expected_motion_x_1,
                                  expected_motion_y_1,
                                  expected_motion_x_2,
                                  expected_motion_y_2)))
        .Times(AnyNumber())
        .WillOnce(mt::WakeUp(&first_client.all_events_received));

    fake_touch_screen->emit_event(mis::a_touch_event()
                                  .at_position({expected_motion_x_1, expected_motion_y_1}));
    // Sleep here to trigger more failures (simulate slow machine)
    // TODO why would that cause failures?b
    std::this_thread::sleep_for(10ms);
    fake_touch_screen->emit_event(mis::a_touch_event()
                                  .with_action(mis::TouchParameters::Action::Move)
                                  .at_position({expected_motion_x_2, expected_motion_y_2}));

    first_client.all_events_received.wait_for(2s);
}

TEST_F(TestClientInput, send_mir_input_events_through_surface)
{
    Client first_client(new_connection(), first);

    EXPECT_CALL(first_client, handle_input(mt::KeyDownEvent()))
        .WillOnce(mt::WakeUp(&first_client.all_events_received));

    auto key_event = mir::events::make_event(MirInputDeviceId{0}, 0ns, std::vector<uint8_t>{}, mir_keyboard_action_down, 0, KEY_M,
                                             mir_input_event_modifier_none);

    server.the_shell()->focused_surface()->consume(key_event.get());

    first_client.all_events_received.wait_for(2s);
}

TEST_F(TestClientInput, clients_receive_keymap_change_events)
{
    Client first_client(new_connection(), first);

    std::string const model = "pc105";
    std::string const layout = "dvorak";
    MirInputDeviceId const id = 1;

    EXPECT_CALL(first_client, handle_keymap(mt::KeymapEventForDevice(id)))
        .Times(1)
        .WillOnce(mt::WakeUp(&first_client.all_events_received));

    server.the_shell()->focused_surface()->set_keymap(id, model, layout, "", "");
    first_client.all_events_received.wait_for(2s);
}

TEST_F(TestClientInput, keymap_changes_change_keycode_received)
{
    Client first_client(new_connection(), first);

    MirInputDeviceId const id = 1;
    std::string const model = "pc105";
    std::string const layout = "us";
    std::string const variant = "dvorak";

    mt::Signal first_event_received,
        client_sees_keymap_change;

    InSequence seq;
    EXPECT_CALL(first_client, handle_input(AllOf(mt::KeyDownEvent(), mt::KeyOfSymbol(XKB_KEY_n))));
    EXPECT_CALL(first_client, handle_input(mt::KeyUpEvent()))
         .WillOnce(mt::WakeUp(&first_event_received));
    EXPECT_CALL(first_client, handle_keymap(mt::KeymapEventForDevice(id)))
        .WillOnce(mt::WakeUp(&client_sees_keymap_change));

    EXPECT_CALL(first_client, handle_input(AllOf(mt::KeyDownEvent(), mt::KeyOfSymbol(XKB_KEY_b))));
    EXPECT_CALL(first_client, handle_input(mt::KeyUpEvent()))
        .WillOnce(mt::WakeUp(&first_client.all_events_received));

    fake_keyboard->emit_event(mis::a_key_down_event().of_scancode(KEY_N));
    fake_keyboard->emit_event(mis::a_key_up_event().of_scancode(KEY_N));

    first_event_received.wait_for(60s);

    server.the_shell()->focused_surface()->set_keymap(id, model, layout, variant, "");

    client_sees_keymap_change.wait_for(60s);

    fake_keyboard->emit_event(mis::a_key_down_event().of_scancode(KEY_N));
    fake_keyboard->emit_event(mis::a_key_up_event().of_scancode(KEY_N));

    first_client.all_events_received.wait_for(5s);
}


TEST_F(TestClientInput, sends_no_wrong_keymaps_to_clients)
{
    Client first_client(new_connection(), first);

    MirInputDeviceId const id = 1;
    std::string const model = "thargoid207";
    std::string const layout = "polaris";

    mt::Signal first_event_received,
        client_sees_keymap_change;

    EXPECT_CALL(first_client, handle_keymap(mt::KeymapEventForDevice(id))).Times(0);

    EXPECT_THROW(
        {server.the_shell()->focused_surface()->set_keymap(id, model, layout, "", "");},
        std::invalid_argument);
}

TEST_F(TestClientInput, event_filter_may_consume_events)
{
    std::shared_ptr<MockEventFilter> mock_event_filter = std::make_shared<MockEventFilter>();
    server.the_composite_event_filter()->append(mock_event_filter);

    Client first_client(new_connection(), first);

    EXPECT_CALL(*mock_event_filter, handle(mt::InputConfigurationEvent())).Times(AnyNumber()).WillRepeatedly(Return(false));

    InSequence seq;
    EXPECT_CALL(*mock_event_filter, handle(_)).WillOnce(Return(true));
    EXPECT_CALL(*mock_event_filter, handle(_)).WillOnce(
        DoAll(mt::WakeUp(&first_client.all_events_received), Return(true)));

    // Since we handle the events in the filter the client should not receive them.
    EXPECT_CALL(first_client, handle_input(_)).Times(0);

    fake_keyboard->emit_event(mis::a_key_down_event().of_scancode(KEY_M));
    fake_keyboard->emit_event(mis::a_key_up_event().of_scancode(KEY_M));

    first_client.all_events_received.wait_for(10s);
}

namespace
{
struct TestClientInputKeyRepeat : public TestClientInput
{
    TestClientInputKeyRepeat()
        : enable_key_repeat("MIR_SERVER_ENABLE_KEY_REPEAT", "true")
    {
    }
    mtf::TemporaryEnvironmentValue enable_key_repeat;
};
}

TEST_F(TestClientInputKeyRepeat, keys_are_repeated_to_clients)
{
    Client first_client(new_connection(), first);

    InSequence seq;
    EXPECT_CALL(first_client, handle_input(AllOf(mt::KeyDownEvent(), mt::KeyOfSymbol(XKB_KEY_Shift_R))));
    EXPECT_CALL(first_client, handle_input(AllOf(mt::KeyRepeatEvent(),
        mt::KeyOfSymbol(XKB_KEY_Shift_R)))).WillOnce(mt::WakeUp(&first_client.all_events_received));
    // Extra repeats before we shut down.
    EXPECT_CALL(first_client, handle_input(mt::KeyRepeatEvent())).Times(AnyNumber());

    fake_keyboard->emit_event(mis::a_key_down_event().of_scancode(KEY_RIGHTSHIFT));

    first_client.all_events_received.wait_for(10s);
}

TEST_F(TestClientInput, pointer_events_pass_through_shaped_out_regions_of_client)
{
    positions[first] = {{0, 0}, {10, 10}};
    
    Client client(new_connection(), first);

    MirRectangle input_rects[] = {{1, 1, 10, 10}};

    auto spec = mir_create_window_spec(client.connection);
    mir_window_spec_set_input_shape(spec, input_rects, 1);
    mir_window_apply_spec(client.window, spec);
    mir_window_spec_release(spec);

    ASSERT_TRUE(shell->wait_for_modify_surface(5s));

    // We verify that we don't receive the first shaped out button event.
    EXPECT_CALL(client, handle_input(mt::PointerEnterEventWithPosition(1, 1)));
    EXPECT_CALL(client, handle_input(mt::ButtonDownEvent(1, 1)))
        .WillOnce(mt::WakeUp(&client.all_events_received));

    fake_mouse->emit_event(mis::a_button_down_event().of_button(BTN_LEFT).with_action(mis::EventAction::Down));
    fake_mouse->emit_event(mis::a_button_up_event().of_button(BTN_LEFT).with_action(mis::EventAction::Up));
    fake_mouse->emit_event(mis::a_pointer_event().with_movement(1, 1));
    fake_mouse->emit_event(mis::a_button_down_event().of_button(BTN_LEFT));

    client.all_events_received.wait_for(10s);
}

MATCHER_P3(ADeviceMatches, name, unique_id, caps, "")
{
    auto one_of_the_devices_matched = false;
    for (size_t i = 0, e = mir_input_config_device_count(arg); i != e; ++i)
    {
        auto dev = mir_input_config_get_device(arg, i);
        if (mir_input_device_get_name(dev) == name &&
            mir_input_device_get_unique_id(dev) == unique_id &&
            mir_input_device_get_capabilities(dev) == caps)
            one_of_the_devices_matched = true;
    }
    return one_of_the_devices_matched;
}

TEST_F(TestClientInput, client_input_config_request_receives_all_attached_devices)
{
    auto con = mir_connect_sync(new_connection().c_str(), first.c_str());
    auto config = mir_connection_create_input_config(con);
    int limit = 10;
    unsigned num_devices = 0u;
    unsigned const expected_devices = 3u;

    for(auto i = 0; i < limit; i++)
    {
        num_devices = mir_input_config_device_count(config);
        if (num_devices == expected_devices)
            break;

        std::this_thread::sleep_for(10ms);
        mir_input_config_release(config);
        config = mir_connection_create_input_config(con);
    }

    ASSERT_THAT(mir_input_config_device_count(config), Eq(expected_devices));

    EXPECT_THAT(config, ADeviceMatches(keyboard_name, keyboard_unique_id,
                                       uint32_t(mir_input_device_capability_keyboard |
                                                mir_input_device_capability_alpha_numeric)));
    EXPECT_THAT(config, ADeviceMatches(mouse_name, mouse_unique_id, mir_input_device_capability_pointer));
    EXPECT_THAT(config, ADeviceMatches(touchscreen_name, touchscreen_unique_id,
                                        uint32_t(mir_input_device_capability_touchscreen |
                                                 mir_input_device_capability_multitouch)));

    mir_input_config_release(config);
    mir_connection_release(con);
}


TEST_F(TestClientInput, callback_function_triggered_on_input_device_addition)
{
    Client a_client(new_connection(), first);
    mt::Signal callback_triggered;
    mir_connection_set_input_config_change_callback(
        a_client.connection,
        [](MirConnection*, void* cond)
        {
            static_cast<mt::Signal*>(cond)->raise();
        },
        static_cast<void*>(&callback_triggered));

    std::string const touchpad{"touchpad"};
    std::string const touchpad_uid{"touchpad"};
    std::unique_ptr<mtf::FakeInputDevice> fake_touchpad{add_fake_input_device(
        mi::InputDeviceInfo{touchpad, touchpad_uid,
                            mi::DeviceCapability::touchpad | mi::DeviceCapability::pointer})};
    wait_for_input_devices_added_to(server);

    callback_triggered.wait_for(1s);
    EXPECT_THAT(callback_triggered.raised(), Eq(true));

    auto config = mir_connection_create_input_config(a_client.connection);
    EXPECT_THAT(mir_input_config_device_count(config), Eq(4u));
    EXPECT_THAT(config, ADeviceMatches(touchpad, touchpad_uid, uint32_t(mir_input_device_capability_touchpad |
                                                                         mir_input_device_capability_pointer)));

    mir_input_config_release(config);
}

TEST_F(TestClientInput, callback_function_triggered_on_input_device_removal)
{
    Client a_client(new_connection(), first);
    mt::Signal callback_triggered;
    mir_connection_set_input_config_change_callback(
        a_client.connection,
        [](MirConnection*, void* cond)
        {
            static_cast<mt::Signal*>(cond)->raise();
        },
        static_cast<void*>(&callback_triggered));

    fake_keyboard->emit_device_removal();
    callback_triggered.wait_for(1s);

    EXPECT_THAT(callback_triggered.raised(), Eq(true));

    auto config = mir_connection_create_input_config(a_client.connection);
    EXPECT_THAT(mir_input_config_device_count(config), Eq(2u));
    mir_input_config_release(config);
}

TEST_F(TestClientInput, key_event_contains_text_to_append)
{
    Client a_client(new_connection(), first);

    EXPECT_CALL(a_client, handle_input(mt::KeyWithText("x")))
        .WillOnce(mt::WakeUp(&a_client.all_events_received));

    fake_keyboard->emit_event(mis::a_key_down_event().of_scancode(KEY_X));
    a_client.all_events_received.wait_for(10s);
}

TEST_F(TestClientInput, key_event_text_applies_shift_modifiers)
{
    Client a_client(new_connection(), first);

    EXPECT_CALL(a_client, handle_input(AllOf(mt::KeyWithText(""), mt::KeyOfSymbol(XKB_KEY_Shift_L))));
    EXPECT_CALL(a_client, handle_input(mt::KeyWithText("W")))
        .WillOnce(mt::WakeUp(&a_client.all_events_received));

    fake_keyboard->emit_event(mis::a_key_down_event().of_scancode(KEY_LEFTSHIFT));
    fake_keyboard->emit_event(mis::a_key_down_event().of_scancode(KEY_W));
    a_client.all_events_received.wait_for(10s);
}

TEST_F(TestClientInput, on_ctrl_c_key_event_text_is_end_of_text)
{
    Client a_client(new_connection(), first);

    EXPECT_CALL(a_client, handle_input(AllOf(mt::KeyWithText(""), mt::KeyOfSymbol(XKB_KEY_Control_R))));
    EXPECT_CALL(a_client, handle_input(mt::KeyWithText("\003")))
        .WillOnce(mt::WakeUp(&a_client.all_events_received));

    fake_keyboard->emit_event(mis::a_key_down_event().of_scancode(KEY_RIGHTCTRL));
    fake_keyboard->emit_event(mis::a_key_down_event().of_scancode(KEY_C));
    a_client.all_events_received.wait_for(10s);
}

TEST_F(TestClientInput, num_lock_is_off_on_startup)
{
    Client a_client(new_connection(), first);

    EXPECT_CALL(a_client, handle_input(mt::KeyOfSymbol(XKB_KEY_KP_Left)))
        .WillOnce(mt::WakeUp(&a_client.all_events_received));

    fake_keyboard->emit_event(mis::a_key_down_event().of_scancode(KEY_KP4));
    a_client.all_events_received.wait_for(10s);
}

TEST_F(TestClientInput, keeps_num_lock_state_after_focus_change)
{
    Client first_client(new_connection(), first);

    {
        Client second_client(new_connection(), second);
        EXPECT_CALL(second_client, handle_input(mt::KeyDownEvent()));
        EXPECT_CALL(second_client, handle_input(mt::KeyUpEvent()))
            .WillOnce(mt::WakeUp(&second_client.all_events_received));

        fake_keyboard->emit_event(mis::a_key_down_event().of_scancode(KEY_NUMLOCK));
        fake_keyboard->emit_event(mis::a_key_up_event().of_scancode(KEY_NUMLOCK));

        second_client.all_events_received.wait_for(10s);
    }

    EXPECT_CALL(first_client, handle_input(mt::KeyOfSymbol(XKB_KEY_KP_4)))
        .WillOnce(mt::WakeUp(&first_client.all_events_received));
    fake_keyboard->emit_event(mis::a_key_down_event().of_scancode(KEY_KP4));
    first_client.all_events_received.wait_for(10s);
}

TEST_F(TestClientInput, reestablishes_num_lock_state_in_client_with_surface_keymap)
{
    Client a_client_with_keymap(new_connection(), first);

    mir::test::Signal keymap_received;
    mir::test::Signal device_state_received;

    EXPECT_CALL(a_client_with_keymap, handle_keymap(_))
        .WillOnce(mt::WakeUp(&keymap_received));
    EXPECT_CALL(a_client_with_keymap,
                handle_input_device_state(
                    mt::DeviceStateWithPressedKeys(std::vector<uint32_t>{KEY_NUMLOCK, KEY_NUMLOCK})))
        .WillOnce(mt::WakeUp(&device_state_received));

    get_surface(first)->set_keymap(MirInputDeviceId{0}, "pc105", "de", "", "");
    keymap_received.wait_for(4s);

    {
        Client a_client(new_connection(), second);

        EXPECT_CALL(a_client, handle_input(mt::KeyDownEvent()));
        EXPECT_CALL(a_client, handle_input(mt::KeyUpEvent()));
        EXPECT_CALL(a_client, handle_input(AllOf(mt::KeyDownEvent(),mt::KeyOfSymbol(XKB_KEY_KP_4))));
        EXPECT_CALL(a_client, handle_input(AllOf(mt::KeyUpEvent(),mt::KeyOfSymbol(XKB_KEY_KP_4))))
            .WillOnce(mt::WakeUp(&a_client.all_events_received));

        fake_keyboard->emit_event(mis::a_key_down_event().of_scancode(KEY_NUMLOCK));
        fake_keyboard->emit_event(mis::a_key_up_event().of_scancode(KEY_NUMLOCK));
        fake_keyboard->emit_event(mis::a_key_down_event().of_scancode(KEY_KP4));
        fake_keyboard->emit_event(mis::a_key_up_event().of_scancode(KEY_KP4));

        a_client.all_events_received.wait_for(10s);
    }
    device_state_received.wait_for(4s);
    EXPECT_CALL(a_client_with_keymap, handle_input(mt::KeyOfSymbol(XKB_KEY_KP_4)))
        .WillOnce(mt::WakeUp(&a_client_with_keymap.all_events_received));

    fake_keyboard->emit_event(mis::a_key_down_event().of_scancode(KEY_KP4));

    a_client_with_keymap.all_events_received.wait_for(10s);
}

TEST_F(TestClientInput, initial_mouse_configuration_can_be_querried)
{
    Client a_client(new_connection(), first);
    auto config = mir_connection_create_input_config(a_client.connection);
    auto mouse = get_device_with_capabilities(config, mir_input_device_capability_pointer);
    ASSERT_THAT(mouse, Ne(nullptr));

    auto pointer_config = mir_input_device_get_pointer_config(mouse);

    EXPECT_THAT(mir_pointer_config_get_acceleration(pointer_config), Eq(mir_pointer_acceleration_none));
    EXPECT_THAT(mir_pointer_config_get_vertical_scroll_scale(pointer_config), Eq(1.0));
    EXPECT_THAT(mir_pointer_config_get_horizontal_scroll_scale(pointer_config), Eq(1.0));

    mir_input_config_release(config);
}

TEST_F(TestClientInput, no_touchpad_config_on_mouse)
{
    Client a_client(new_connection(), first);
    auto config = mir_connection_create_input_config(a_client.connection);
    auto mouse = get_device_with_capabilities(config, mir_input_device_capability_pointer);

    EXPECT_THAT(mir_input_device_get_touchpad_config(mouse), Eq(nullptr));
    mir_input_config_release(config);
}

TEST_F(TestClientInput, pointer_config_is_mutable)
{
    Client a_client(new_connection(), first);
    auto config = mir_connection_create_input_config(a_client.connection);
    auto mouse = get_mutable_device_with_capabilities(config, mir_input_device_capability_pointer);
    auto pointer_config = mir_input_device_get_mutable_pointer_config(mouse);

    mir_pointer_config_set_acceleration(pointer_config, mir_pointer_acceleration_adaptive);
    mir_pointer_config_set_acceleration_bias(pointer_config, 1.0);
    mir_pointer_config_set_horizontal_scroll_scale(pointer_config, 1.2);
    mir_pointer_config_set_vertical_scroll_scale(pointer_config, 1.4);

    EXPECT_THAT(mir_pointer_config_get_vertical_scroll_scale(pointer_config), Eq(1.4));
    EXPECT_THAT(mir_pointer_config_get_horizontal_scroll_scale(pointer_config), Eq(1.2));
    EXPECT_THAT(mir_pointer_config_get_acceleration(pointer_config), Eq(mir_pointer_acceleration_adaptive));
    EXPECT_THAT(mir_pointer_config_get_acceleration_bias(pointer_config), Eq(1.0));

    mir_input_config_release(config);
}

TEST_F(TestClientInput, touchpad_config_can_be_querried)
{
    MirTouchpadConfig const default_configuration;
    std::unique_ptr<mtf::FakeInputDevice> fake_touchpad{add_fake_input_device(
        mi::InputDeviceInfo{"tpd", "tpd-id",
                            mi::DeviceCapability::pointer | mi::DeviceCapability::touchpad})};
    wait_for_input_devices_added_to(server);

    Client a_client(new_connection(), first);
    auto config = mir_connection_create_input_config(a_client.connection);
    auto touchpad = get_device_with_capabilities(config,
                                                 mir_input_device_capability_pointer|
                                                 mir_input_device_capability_touchpad);
    ASSERT_THAT(touchpad, Ne(nullptr));

    auto touchpad_config = mir_input_device_get_touchpad_config(touchpad);

    EXPECT_THAT(mir_touchpad_config_get_click_modes(touchpad_config), Eq(default_configuration.click_mode()));
    EXPECT_THAT(mir_touchpad_config_get_scroll_modes(touchpad_config), Eq(default_configuration.scroll_mode()));
    EXPECT_THAT(mir_touchpad_config_get_button_down_scroll_button(touchpad_config), Eq(default_configuration.button_down_scroll_button()));
    EXPECT_THAT(mir_touchpad_config_get_tap_to_click(touchpad_config), Eq(default_configuration.tap_to_click()));
    EXPECT_THAT(mir_touchpad_config_get_middle_mouse_button_emulation(touchpad_config), Eq(default_configuration.middle_mouse_button_emulation()));
    EXPECT_THAT(mir_touchpad_config_get_disable_with_mouse(touchpad_config), Eq(default_configuration.disable_with_mouse()));
    EXPECT_THAT(mir_touchpad_config_get_disable_while_typing(touchpad_config), Eq(default_configuration.disable_while_typing()));

    mir_input_config_release(config);
}

TEST_F(TestClientInput, touchpad_config_is_mutable)
{
    std::unique_ptr<mtf::FakeInputDevice> fake_touchpad{add_fake_input_device(
        mi::InputDeviceInfo{"tpd", "tpd-id",
                            mi::DeviceCapability::pointer | mi::DeviceCapability::touchpad})};
    wait_for_input_devices_added_to(server);

    Client a_client(new_connection(), first);
    auto config = mir_connection_create_input_config(a_client.connection);
    auto touchpad = get_mutable_device_with_capabilities(config,
                                                 mir_input_device_capability_pointer|
                                                 mir_input_device_capability_touchpad);
    ASSERT_THAT(touchpad, Ne(nullptr));

    auto touchpad_config = mir_input_device_get_mutable_touchpad_config(touchpad);

    mir_touchpad_config_set_click_modes(touchpad_config, mir_touchpad_click_mode_area_to_click);
    mir_touchpad_config_set_scroll_modes(touchpad_config, mir_touchpad_scroll_mode_edge_scroll|mir_touchpad_scroll_mode_button_down_scroll);
    mir_touchpad_config_set_button_down_scroll_button(touchpad_config, 10);
    mir_touchpad_config_set_tap_to_click(touchpad_config, true);
    mir_touchpad_config_set_middle_mouse_button_emulation(touchpad_config, true);
    mir_touchpad_config_set_disable_with_mouse(touchpad_config, true);
    mir_touchpad_config_set_disable_while_typing(touchpad_config, false);

    EXPECT_THAT(mir_touchpad_config_get_click_modes(touchpad_config), Eq(mir_touchpad_click_mode_area_to_click));
    EXPECT_THAT(mir_touchpad_config_get_scroll_modes(touchpad_config),
                Eq(static_cast<MirTouchpadClickModes>(
                    mir_touchpad_scroll_mode_edge_scroll
                  | mir_touchpad_scroll_mode_button_down_scroll)));
    EXPECT_THAT(mir_touchpad_config_get_button_down_scroll_button(touchpad_config), Eq(10));
    EXPECT_THAT(mir_touchpad_config_get_tap_to_click(touchpad_config), Eq(true));
    EXPECT_THAT(mir_touchpad_config_get_middle_mouse_button_emulation(touchpad_config), Eq(true));
    EXPECT_THAT(mir_touchpad_config_get_disable_with_mouse(touchpad_config), Eq(true));
    EXPECT_THAT(mir_touchpad_config_get_disable_while_typing(touchpad_config), Eq(false));

    mir_input_config_release(config);
}

TEST_F(TestClientInput, clients_can_apply_changed_input_configuration)
{
    Client a_client(new_connection(), first);
    auto config = mir_connection_create_input_config(a_client.connection);
    auto mouse = get_mutable_device_with_capabilities(config, mir_input_device_capability_pointer);
    auto pointer_config = mir_input_device_get_mutable_pointer_config(mouse);
    auto increased_acceleration = 0.7f;

    mir_pointer_config_set_acceleration(pointer_config, mir_pointer_acceleration_adaptive);
    mir_pointer_config_set_acceleration_bias(pointer_config, increased_acceleration);

    apply_and_wait_for_completion(
        a_client.connection,
        mir_connection_set_input_config_change_callback,
        [config](MirConnection* connection)
        {
            mir_connection_apply_session_input_config(connection, config);
            mir_input_config_release(config);
        });

    config = mir_connection_create_input_config(a_client.connection);
    mouse = get_mutable_device_with_capabilities(config, mir_input_device_capability_pointer);
    pointer_config = mir_input_device_get_mutable_pointer_config(mouse);

    EXPECT_THAT(mir_pointer_config_get_acceleration(pointer_config), Eq(mir_pointer_acceleration_adaptive));
    EXPECT_THAT(mir_pointer_config_get_acceleration_bias(pointer_config), Eq(increased_acceleration));
    mir_input_config_release(config);
}

TEST_F(TestClientInput, keyboard_config_can_be_querried)
{
    Client a_client(new_connection(), first);
    auto config = mir_connection_create_input_config(a_client.connection);
    auto keyboard = get_mutable_device_with_capabilities(
        config, mir_input_device_capability_keyboard | mir_input_device_capability_alpha_numeric);
    ASSERT_THAT(keyboard, Ne(nullptr));
    auto keyboard_config = mir_input_device_get_keyboard_config(keyboard);

    EXPECT_THAT(mir_keyboard_config_get_keymap_model(keyboard_config), StrEq("pc105+inet"));
    EXPECT_THAT(mir_keyboard_config_get_keymap_layout(keyboard_config), StrEq("us"));
    EXPECT_THAT(mir_keyboard_config_get_keymap_variant(keyboard_config), StrEq(""));
    EXPECT_THAT(mir_keyboard_config_get_keymap_options(keyboard_config), StrEq(""));

    mir_input_config_release(config);
}

TEST_F(TestClientInput, keyboard_config_is_mutable)
{
    Client a_client(new_connection(), first);
    auto config = mir_connection_create_input_config(a_client.connection);
    auto keyboard = get_mutable_device_with_capabilities(
        config, mir_input_device_capability_keyboard | mir_input_device_capability_alpha_numeric);
    ASSERT_THAT(keyboard, Ne(nullptr));
    auto keyboard_config = mir_input_device_get_mutable_keyboard_config(keyboard);

    mir_keyboard_config_set_keymap_model(keyboard_config, "pc104");
    mir_keyboard_config_set_keymap_layout(keyboard_config, "fr");
    mir_keyboard_config_set_keymap_variant(keyboard_config, "alt");
    mir_keyboard_config_set_keymap_options(keyboard_config, "compose:ralt");

    EXPECT_THAT(mir_keyboard_config_get_keymap_model(keyboard_config), StrEq("pc104"));
    EXPECT_THAT(mir_keyboard_config_get_keymap_layout(keyboard_config), StrEq("fr"));
    EXPECT_THAT(mir_keyboard_config_get_keymap_variant(keyboard_config), StrEq("alt"));
    EXPECT_THAT(mir_keyboard_config_get_keymap_options(keyboard_config), StrEq("compose:ralt"));

    mir_input_config_release(config);
}

TEST_F(TestClientInput, keyboard_config_can_be_changed)
{
    Client a_client(new_connection(), first);
    auto config = mir_connection_create_input_config(a_client.connection);
    auto keyboard = get_mutable_device_with_capabilities(
        config, mir_input_device_capability_keyboard | mir_input_device_capability_alpha_numeric);
    ASSERT_THAT(keyboard, Ne(nullptr));

    auto keyboard_config = mir_input_device_get_mutable_keyboard_config(keyboard);

    mir_keyboard_config_set_keymap_layout(keyboard_config, "de");

    mt::Signal changes_complete;
    mir_connection_set_input_config_change_callback(
        a_client.connection,
        [](MirConnection*, void* context)
        {
            static_cast<mt::Signal*>(context)->raise();
        },
        &changes_complete
        );
    mir_connection_apply_session_input_config(a_client.connection, config);
    mir_input_config_release(config);

    EXPECT_TRUE(changes_complete.wait_for(10s));
    EXPECT_CALL(a_client, handle_input(mt::KeyOfSymbol(XKB_KEY_y)))
        .WillOnce(mt::WakeUp(&a_client.all_events_received));

    fake_keyboard->emit_event(mis::a_key_down_event().of_scancode(KEY_Z));

    EXPECT_TRUE(a_client.all_events_received.wait_for(10s));
}

TEST_F(TestClientInput, unfocused_client_can_change_base_configuration)
{
    Client unfocused_client(new_connection(), first);
    Client focused_client(new_connection(), second);
    auto config = mir_connection_create_input_config(unfocused_client.connection);
    auto mouse = get_mutable_device_with_capabilities(config, mir_input_device_capability_pointer);
    auto pointer_config = mir_input_device_get_mutable_pointer_config(mouse);

    mir_pointer_config_set_acceleration(pointer_config, mir_pointer_acceleration_adaptive);

    apply_and_wait_for_completion(
        unfocused_client.connection,
        mir_connection_set_input_config_change_callback,
        [config](MirConnection* connection)
        {
            mir_connection_set_base_input_config(connection, config);
            mir_input_config_release(config);
        });

    config = mir_connection_create_input_config(unfocused_client.connection);
    mouse = get_mutable_device_with_capabilities(config, mir_input_device_capability_pointer);
    pointer_config = mir_input_device_get_mutable_pointer_config(mouse);

    EXPECT_THAT(mir_pointer_config_get_acceleration(pointer_config), Eq(mir_pointer_acceleration_adaptive));
    mir_input_config_release(config);
}

TEST_F(TestClientInput, unfocused_client_cannot_change_input_configuration)
{
    Client unfocused_client(new_connection(), first);
    Client focused_client(new_connection(), second);
    auto config = mir_connection_create_input_config(unfocused_client.connection);
    auto mouse = get_mutable_device_with_capabilities(config, mir_input_device_capability_pointer);
    auto pointer_config = mir_input_device_get_mutable_pointer_config(mouse);

    mir_pointer_config_set_acceleration(pointer_config, mir_pointer_acceleration_adaptive);

    mt::Signal expect_no_changes;
    mir_connection_set_input_config_change_callback(
        unfocused_client.connection,
        [](MirConnection*, void* context)
        {
            static_cast<mt::Signal*>(context)->raise();
        },
        &expect_no_changes
        );
    mir_connection_apply_session_input_config(unfocused_client.connection, config);
    mir_input_config_release(config);

    EXPECT_FALSE(expect_no_changes.wait_for(1s));
    mir_connection_set_input_config_change_callback(unfocused_client.connection, [](MirConnection*, void*){}, nullptr);
}

TEST_F(TestClientInput, focused_client_can_change_base_configuration)
{
    Client focused_client(new_connection(), second);
    auto config = mir_connection_create_input_config(focused_client.connection);
    auto mouse = get_mutable_device_with_capabilities(config, mir_input_device_capability_pointer);
    auto pointer_config = mir_input_device_get_mutable_pointer_config(mouse);

    mir_pointer_config_set_acceleration(pointer_config, mir_pointer_acceleration_adaptive);

    apply_and_wait_for_completion(
        focused_client.connection,
        mir_connection_set_input_config_change_callback,
        [config](MirConnection* connection)
        {
            mir_connection_set_base_input_config(connection, config);
            mir_input_config_release(config);
        });

    config = mir_connection_create_input_config(focused_client.connection);
    mouse = get_mutable_device_with_capabilities(config, mir_input_device_capability_pointer);
    pointer_config = mir_input_device_get_mutable_pointer_config(mouse);

    EXPECT_THAT(mir_pointer_config_get_acceleration(pointer_config), Eq(mir_pointer_acceleration_adaptive));
    mir_input_config_release(config);
}

TEST_F(TestClientInput, set_base_configuration_for_unauthorized_client_fails)
{
    stub_authorizer.allow_set_base_input_configuration = false;

    Client unauthed_client(new_connection(), second);
    auto config = mir_connection_create_input_config(unauthed_client.connection);

    mt::Signal wait_for_error;
    mir_connection_set_error_callback(
        unauthed_client.connection,
        [](MirConnection*, MirError const* error, void* context)
        {
            if (mir_error_get_domain(error) == mir_error_domain_input_configuration &&
                mir_error_get_code(error) == mir_input_configuration_error_base_configuration_unauthorized)
                static_cast<mt::Signal*>(context)->raise();
        },
        &wait_for_error);
    mir_connection_set_base_input_config(unauthed_client.connection, config);
    mir_input_config_release(config);

    EXPECT_TRUE(wait_for_error.wait_for(10s));
}

TEST_F(TestClientInput, set_configuration_for_unauthorized_client_fails)
{
    stub_authorizer.allow_configure_input = false;

    Client unauthed_client(new_connection(), second);
    auto config = mir_connection_create_input_config(unauthed_client.connection);

    mt::Signal wait_for_error;
    mir_connection_set_error_callback(
        unauthed_client.connection,
        [](MirConnection*, MirError const* error, void* context)
        {
            if (mir_error_get_domain(error) == mir_error_domain_input_configuration &&
                mir_error_get_code(error) == mir_input_configuration_error_unauthorized)
                static_cast<mt::Signal*>(context)->raise();
        },
        &wait_for_error);
    mir_connection_apply_session_input_config(unauthed_client.connection, config);
    mir_input_config_release(config);

    EXPECT_TRUE(wait_for_error.wait_for(10s));
}

TEST_F(TestClientInput, error_callback_triggered_on_wrong_configuration)
{
    Client a_client(new_connection(), first);
    auto config = mir_connection_create_input_config(a_client.connection);
    auto mouse = get_mutable_device_with_capabilities(config, mir_input_device_capability_pointer);
    auto pointer_config = mir_input_device_get_mutable_pointer_config(mouse);

    float out_of_range = 3.0f;
    mir_pointer_config_set_acceleration_bias(pointer_config, out_of_range);

    mt::Signal wait_for_error;
    mir_connection_set_error_callback(
        a_client.connection,
        [](MirConnection*, MirError const* error, void* context)
        {
            if (mir_error_get_domain(error) == mir_error_domain_input_configuration &&
                mir_error_get_code(error) == mir_input_configuration_error_rejected_by_driver)
                static_cast<mt::Signal*>(context)->raise();
        },
        &wait_for_error);
    mir_connection_apply_session_input_config(a_client.connection, config);
    mir_input_config_release(config);

    EXPECT_TRUE(wait_for_error.wait_for(10s));
}

TEST_F(TestClientInput, touchscreen_config_is_mutable)
{
    Client a_client(new_connection(), first);
    auto config = mir_connection_create_input_config(a_client.connection);
    auto touchscreen = get_mutable_device_with_capabilities(config,
                                                 mir_input_device_capability_touchscreen|
                                                 mir_input_device_capability_multitouch);
    ASSERT_THAT(touchscreen, Ne(nullptr));

    auto touchscreen_config = mir_input_device_get_mutable_touchscreen_config(touchscreen);

    mir_touchscreen_config_set_output_id(touchscreen_config, 4);
    mir_touchscreen_config_set_mapping_mode(touchscreen_config, mir_touchscreen_mapping_mode_to_display_wall);

    EXPECT_THAT(mir_touchscreen_config_get_mapping_mode(touchscreen_config), Eq(mir_touchscreen_mapping_mode_to_display_wall));
    EXPECT_THAT(mir_touchscreen_config_get_output_id(touchscreen_config), Eq(4));

    mir_input_config_release(config);
}

TEST_F(TestClientInputWithTwoScreens, touchscreen_can_be_mapped_to_second_output)
{
    uint32_t const second_output = 2;
    int const touch_x = 10;
    int const touch_y = 10;

    int const expected_x = touch_x + second_screen.top_left.x.as_int();
    int const expected_y = touch_y + second_screen.top_left.y.as_int();

    mt::Signal touchscreen_ready;
    fake_touch_screen->on_new_configuration_do(
        [&touchscreen_ready](mi::InputDevice const& dev)
        {
            auto ts = dev.get_touchscreen_settings();
            if (ts.is_set() && ts.value().output_id == second_output)
                touchscreen_ready.raise();
        });

    Client client(new_connection(), first);
    auto config = mir_connection_create_input_config(client.connection);
    auto touchscreen = get_mutable_device_with_capabilities(config,
                                                            mir_input_device_capability_touchscreen|
                                                            mir_input_device_capability_multitouch);
    auto touchscreen_config = mir_input_device_get_mutable_touchscreen_config(touchscreen);
    mir_touchscreen_config_set_output_id(touchscreen_config, second_output);
    mir_touchscreen_config_set_mapping_mode(touchscreen_config, mir_touchscreen_mapping_mode_to_output);

    apply_and_wait_for_completion(
        client.connection,
        mir_connection_set_input_config_change_callback,
        [config](MirConnection* connection)
        {
            mir_connection_set_base_input_config(connection, config);
            mir_input_config_release(config);
        });

    EXPECT_TRUE(touchscreen_ready.wait_for(10s));
    EXPECT_CALL(client, handle_input(mt::TouchEvent(expected_x, expected_y)))
        .WillOnce(mt::WakeUp(&client.all_events_received));
    fake_touch_screen->emit_event(mis::a_touch_event()
                           .at_position({touch_x, touch_y}));

    EXPECT_TRUE(client.all_events_received.wait_for(10s));
}

TEST_F(TestClientInputWithTwoScreens, touchscreen_mapped_to_deactivated_output_is_filtered_out)
{
    uint32_t const second_output = 2;
    int const touch_x = 10;
    int const touch_y = 10;

    int const expected_x = second_screen.top_left.x.as_int() + touch_x;
    int const expected_y = second_screen.top_left.y.as_int() + touch_y;

    Client client(new_connection(), first);
    auto display_config = mir_connection_create_display_configuration(client.connection);
    auto second_output_ptr = mir_display_config_get_mutable_output(display_config, 1);
    mir_output_set_power_mode(second_output_ptr, mir_power_mode_off);

    apply_and_wait_for_completion(
        client.connection,
        mir_connection_set_display_config_change_callback,
        [display_config](MirConnection* con)
        {
            mir_connection_preview_base_display_configuration(con, display_config, 10);
        });

    apply_and_wait_for_completion(
        client.connection,
        mir_connection_set_display_config_change_callback,
        [display_config](MirConnection* con)
        {
            mir_connection_confirm_base_display_configuration(con, display_config);
            mir_display_config_release(display_config);
        });

    display_config = mir_connection_create_display_configuration(client.connection);
    ASSERT_THAT(mir_output_get_power_mode(mir_display_config_get_output(display_config, 1)), Eq(mir_power_mode_off));
    mir_display_config_release(display_config);

    mt::Signal touchscreen_ready;
    fake_touch_screen->on_new_configuration_do(
        [&touchscreen_ready](mi::InputDevice const& dev)
        {
            auto ts = dev.get_touchscreen_settings();
            if (ts.is_set()
                && ts.value().output_id == second_output
                && ts.value().mapping_mode == mir_touchscreen_mapping_mode_to_output)
                touchscreen_ready.raise();
        });

    auto config = mir_connection_create_input_config(client.connection);
    auto touchscreen = get_mutable_device_with_capabilities(config,
                                                            mir_input_device_capability_touchscreen|
                                                            mir_input_device_capability_multitouch);
    auto touchscreen_config = mir_input_device_get_mutable_touchscreen_config(touchscreen);

    mir_touchscreen_config_set_output_id(touchscreen_config, second_output);
    mir_touchscreen_config_set_mapping_mode(touchscreen_config, mir_touchscreen_mapping_mode_to_output);

    apply_and_wait_for_completion(
        client.connection,
        mir_connection_set_input_config_change_callback,
        [config](MirConnection* connection)
        {
            mir_connection_set_base_input_config(connection, config);
            mir_input_config_release(config);
        });

    EXPECT_TRUE(touchscreen_ready.wait_for(10s));
    ON_CALL(client, handle_input(mt::TouchEvent(expected_x, expected_y)))
        .WillByDefault(mt::WakeUp(&client.all_events_received));
    fake_touch_screen->emit_event(mis::a_touch_event()
                           .at_position({touch_x, touch_y}));

    EXPECT_FALSE(client.all_events_received.wait_for(5s));
}
