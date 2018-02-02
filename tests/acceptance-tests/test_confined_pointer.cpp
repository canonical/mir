/*
 * Copyright © 2016 Canonical Ltd.
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
 * Authored by: Brandon Schaefer <brandon.schaefer@canonical.com>
 */

#include "mir/scene/null_surface_observer.h"
#include "mir/input/input_device_info.h"
#include "mir/geometry/rectangles.h"
#include "mir/observer_registrar.h"

#include "mir/test/event_matchers.h"
#include "mir/test/fake_shared.h"
#include "mir/test/signal.h"
#include "mir/test/doubles/mock_seat_report.h"
#include "mir_test_framework/fake_input_device.h"
#include "mir_test_framework/headless_in_process_server.h"
#include "mir_test_framework/placement_applying_shell.h"
#include "mir_test_framework/stub_server_platform_factory.h"

#include "mir_toolkit/mir_client_library.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <boost/throw_exception.hpp>

namespace mtd = mir::test::doubles;
namespace mt = mir::test;
namespace mi = mir::input;
namespace ms = mir::scene;
namespace mis = mir::input::synthesis;
namespace mtf = mir_test_framework;
namespace geom = mir::geometry;

using namespace std::chrono_literals;
using namespace testing;

namespace
{
int const surface_width  = 100;
int const surface_height = 100;

struct MockSurfaceObserver : public ms::NullSurfaceObserver
{
    MOCK_METHOD2(resized_to, void(ms::Surface const*, geom::Size const&));
};
}

void null_event_handler(MirWindow*, MirEvent const*, void*)
{
}

struct Client
{
    MOCK_METHOD1(handle_input, void(MirEvent const*));

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
#pragma GCC diagnostic pop
        mir_window_spec_set_pointer_confinement(spec, mir_pointer_confined_to_window);
        mir_window_spec_set_name(spec, name.c_str());
        mir_window_spec_set_event_handler(spec, handle_event, this);
        window = mir_create_window_sync(spec);
        mir_window_spec_release(spec);
        if (!mir_window_is_valid(window))
        {
            BOOST_THROW_EXCEPTION(std::runtime_error{std::string{"Failed creating a window: "}+
                mir_window_get_error_message(window)});
        }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
        mir_buffer_stream_swap_buffers_sync(
            mir_window_get_buffer_stream(window));
#pragma GCC diagnostic pop
        ready_to_accept_events.wait_for(4s);
        if (!ready_to_accept_events.raised())
        {
            BOOST_THROW_EXCEPTION(std::runtime_error("Timeout waiting for window to become focused and exposed"));
        }
    }

    void resize(int width, int height)
    {
        auto spec = mir_create_window_spec(connection);
        mir_window_spec_set_width (spec, width);
        mir_window_spec_set_height(spec, height);

        mir_window_apply_spec(window, spec);
        mir_window_spec_release(spec);
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

        if (exposed && focused)
            ready_to_accept_events.raise();
    }

    static void handle_event(MirWindow*, MirEvent const* ev, void* context)
    {
        auto const client = static_cast<Client*>(context);
        auto type = mir_event_get_type(ev);
        switch (type)
        {
        case mir_event_type_window:
            client->handle_window_event(mir_event_get_window_event(ev));
            break;
        case mir_event_type_input:
            client->handle_input(ev);
            break;
        default:
            break;
        }
    }
    ~Client()
    {
        // Remove the event handler to avoid handling spurious events unrelated
        // to the tests (e.g. pointer leave events when the window is destroyed),
        // which can cause test expectations to fail.
        mir_window_set_event_handler(window, null_event_handler, nullptr);
        mir_window_release_sync(window);
        mir_connection_release(connection);
    }

    MirWindow* window{nullptr};
    MirConnection* connection;
    mir::test::Signal ready_to_accept_events;
    mir::test::Signal all_events_received;
    bool exposed = false;
    bool focused = false;
};

struct PointerConfinement : mtf::HeadlessInProcessServer
{
    void SetUp() override
    {
        initial_display_layout({screen_geometry});

        server.wrap_shell(
            [this](std::shared_ptr<mir::shell::Shell> const& wrapped)
            {
                shell = std::make_shared<mtf::PlacementApplyingShell>(wrapped, input_regions, positions);
                return shell;
            });

        HeadlessInProcessServer::SetUp();

        server.the_seat_observer_registrar()->register_interest(mock_seat_observer);

        positions[first] = geom::Rectangle{{0,0}, {surface_width, surface_height}};
    }

    std::shared_ptr<ms::Surface> latest_shell_surface() const
    {
        auto const result = shell->latest_surface.lock();
//      ASSERT_THAT(result, NotNull()); //<= doesn't compile!?
        EXPECT_THAT(result, NotNull());
        return result;
    }

    void change_observed() { resized_signaled.raise(); }

    std::string const mouse_name = "mouse";
    std::string const mouse_unique_id = "mouse-uid";
    std::unique_ptr<mtf::FakeInputDevice> fake_mouse{
        mtf::add_fake_input_device(mi::InputDeviceInfo{mouse_name, mouse_unique_id, mi::DeviceCapability::pointer})};

    NiceMock<MockSurfaceObserver> surface_observer;
    mir::test::Signal resized_signaled;

    std::shared_ptr<mtd::MockSeatObserver> mock_seat_observer{std::make_shared<NiceMock<mtd::MockSeatObserver>>()};
    std::shared_ptr<mtf::PlacementApplyingShell> shell;
    geom::Rectangle screen_geometry{{0,0}, {800,600}};
    mtf::ClientInputRegions input_regions;
    std::string first{"first"};
    std::string second{"second"};
    mtf::ClientPositions positions;
};

TEST_F(PointerConfinement, test_we_hit_pointer_confined_boundary)
{
    positions[first] = geom::Rectangle{{0,0}, {surface_width, surface_height}};
    Client client(new_connection(), first);

    InSequence seq;
    EXPECT_CALL(client, handle_input(mt::PointerEnterEventWithPosition(surface_width / 2, 0)));
    EXPECT_CALL(client, handle_input(AllOf(mt::PointerEventWithPosition(surface_width - 1, 0),
                                           mt::PointerEventWithDiff(surface_width * 2, 0))));
    EXPECT_CALL(client, handle_input(AllOf(mt::PointerEventWithPosition(surface_width - 1, 0),
                                           mt::PointerEventWithDiff(10, 0))))
        .WillOnce(mt::WakeUp(&client.all_events_received));

    fake_mouse->emit_event(mis::a_pointer_event().with_movement(surface_width / 2, 0));
    fake_mouse->emit_event(mis::a_pointer_event().with_movement(surface_width * 2, 0));
    fake_mouse->emit_event(mis::a_pointer_event().with_movement(10, 0));

    client.all_events_received.wait_for(10s);
}

TEST_F(PointerConfinement, test_we_generate_relative_after_boundary)
{
    positions[first] = geom::Rectangle{{0,0}, {surface_width, surface_height}};
    Client client(new_connection(), first);

    InSequence seq;
    EXPECT_CALL(client, handle_input(mt::PointerEnterEventWithPosition(surface_width - 1, 0)));
    EXPECT_CALL(client, handle_input(AllOf(mt::PointerEventWithPosition(surface_width - 1, 0), mt::PointerEventWithDiff(10, 0))))
        .WillOnce(mt::WakeUp(&client.all_events_received));

    fake_mouse->emit_event(mis::a_pointer_event().with_movement(surface_width + 1, 0));
    fake_mouse->emit_event(mis::a_pointer_event().with_movement(10, 0));

    client.all_events_received.wait_for(10s);
}

TEST_F(PointerConfinement, test_we_update_our_confined_region_on_a_resize)
{
    positions[first] = geom::Rectangle{{0,0}, {surface_width, surface_height}};

    geom::Rectangles main_region{{positions[first]}};
    EXPECT_CALL(*mock_seat_observer, seat_set_confinement_region_called(mt::RectanglesMatches(main_region)))
        .Times(1);

    Client client(new_connection(), first);

    auto fake = mt::fake_shared(surface_observer);
    latest_shell_surface()->add_observer(fake);

    geom::Size new_size = {surface_width * 2, surface_height};
    EXPECT_CALL(surface_observer, resized_to(_, new_size)).Times(1);

    geom::Rectangles confinement_region{{{0, 0}, new_size}};
    EXPECT_CALL(*mock_seat_observer, seat_set_confinement_region_called(mt::RectanglesMatches(confinement_region)))
        .WillRepeatedly(InvokeWithoutArgs([&] { change_observed(); }));

    client.resize(surface_width * 2, surface_height);
    resized_signaled.wait_for(1s);

    InSequence seq;
    EXPECT_CALL(client, handle_input(mt::PointerEnterEventWithPosition(surface_width * 2 - 1, 0)));
    EXPECT_CALL(client, handle_input(AllOf(mt::PointerEventWithPosition(surface_width * 2 - 1, 0), mt::PointerEventWithDiff(10, 0))))
        .WillOnce(mt::WakeUp(&client.all_events_received));

    fake_mouse->emit_event(mis::a_pointer_event().with_movement(surface_width * 2 + 1, 0));
    fake_mouse->emit_event(mis::a_pointer_event().with_movement(10, 0));

    client.all_events_received.wait_for(10s);
}

TEST_F(PointerConfinement, cannot_confine_to_unfocused_surface)
{
    positions[first] = geom::Rectangle{{0,0}, {surface_width, surface_height}};
    positions[second] = geom::Rectangle{{surface_width / 2, surface_width / 2},
                                        {surface_width, surface_height}};
    Client client_1(new_connection(), first);
    Client client_2(new_connection(), second);

    // Attempt to confine client_1 while client_2 is focused
    auto spec = mir_create_window_spec(client_1.connection);
    mir_window_spec_set_pointer_confinement(spec, mir_pointer_confined_to_window);

    mir_window_apply_spec(client_1.window, spec);
    mir_window_spec_release(spec);

    // We have to wait since we *wont* set the seat here
    std::this_thread::sleep_for(1s);

    InSequence seq;
    EXPECT_CALL(client_2, handle_input(mt::PointerEnterEventWithPosition(0, 0)));
    EXPECT_CALL(client_2, handle_input(mt::PointerEventWithPosition(surface_width - 1, 0)))
        .WillOnce(mt::WakeUp(&client_2.all_events_received));

    // Move to top_left of our client_2
    fake_mouse->emit_event(mis::a_pointer_event().with_movement(surface_width / 2, surface_height / 2));
    // Move from top_left of our client_2 to the top_left.x + size.width of client_2
    fake_mouse->emit_event(mis::a_pointer_event().with_movement(surface_width, 0));

    client_2.all_events_received.wait_for(10s);
}

