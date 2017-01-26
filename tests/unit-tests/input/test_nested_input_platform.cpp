/*
 * Copyright © 2016 Canonical Ltd.
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

#include "src/server/graphics/nested/input_platform.h"
#include "src/server/report/null_report_factory.h"
#include "src/include/client/mir/input/input_devices.h"
#include "src/server/input/default_event_builder.h"

#include "mir/cookie/authority.h"
#include "mir/input/device_capability.h"
#include "mir/input/input_device.h"
#include "mir/input/input_device_info.h"
#include "mir/input/input_report.h"
#include "mir/input/mir_input_config.h"
#include "mir/events/event_builders.h"
#include "mir_toolkit/mir_connection.h"
#include "mir_toolkit/mir_input_device.h"

#include "mir/test/doubles/mock_input_device_registry.h"
#include "mir/test/doubles/mock_input_sink.h"
#include "mir/test/doubles/mock_input_seat.h"
#include "mir/test/doubles/stub_host_connection.h"
#include "mir/test/fake_shared.h"
#include "mir/test/event_matchers.h"

namespace mi = mir::input;
namespace mt = mir::test;
namespace mr = mir::report;
namespace mev = mir::events;
namespace mtd = mir::test::doubles;
namespace mgn = mir::graphics::nested;
using namespace std::chrono_literals;
using namespace testing;

namespace
{

struct TestNestedInputPlatform : Test
{
    NiceMock<mtd::MockInputDeviceRegistry> mock_input_device_registry;
    NiceMock<mtd::MockHostConnection> mock_host_connection;
    NiceMock<mtd::MockInputSeat> mock_seat;
    mgn::InputPlatform platform{mt::fake_shared(mock_host_connection), mt::fake_shared(mock_input_device_registry),
                                mr::null_input_report()};
    MirInputDevice a_keyboard{1, mi::DeviceCapabilities{mir_input_device_capability_keyboard}, "keys" , "keys-evdev2"};
    MirInputDevice a_mouse{0, mi::DeviceCapabilities{mir_input_device_capability_pointer}, "rodent", "rodent-evdev1"};
    const mir::geometry::Rectangle source_surface{{0, 0}, {100, 100}};

    auto capture_input_device(MirInputDevice& dev)
    {
        std::shared_ptr<mi::InputDevice> input_dev;
        ON_CALL(mock_input_device_registry, add_device(_))
            .WillByDefault(SaveArg<0>(&input_dev));
        platform.start();
        MirInputConfig config;
        config.add_device_config(dev);
        mgn::UniqueInputConfig input_config(
            new MirInputConfig(config),
            mir_input_config_release);
        mock_host_connection.device_change_callback(std::move(input_config));

        platform.dispatchable()->dispatch(mir::dispatch::readable);

        return input_dev;
    }
};

}

TEST_F(TestNestedInputPlatform, registers_to_host_connection)
{
    EXPECT_CALL(mock_host_connection, set_input_device_change_callback(_)).Times(1);
    EXPECT_CALL(mock_host_connection, set_input_event_callback(_));

    platform.start();
}

MATCHER_P(MatchesDeviceData, device_data, "")
{
    mi::InputDevice& dev = *arg;
    auto dev_info = dev.get_device_info();

    if (dev_info.name != device_data.name())
        return false;
    if (dev_info.unique_id != device_data.unique_id())
        return false;
    if (dev_info.capabilities != device_data.capabilities())
        return false;

    return true;
}

TEST_F(TestNestedInputPlatform, registers_devices)
{
    EXPECT_CALL(mock_input_device_registry, add_device(MatchesDeviceData(a_keyboard)));
    EXPECT_CALL(mock_input_device_registry, add_device(MatchesDeviceData(a_mouse)));

    platform.start();
    MirInputConfig devices;
    devices.add_device_config(a_keyboard);
    devices.add_device_config(a_mouse);
    mgn::UniqueInputConfig input_config(
        new MirInputConfig(devices),
        mir_input_config_release);
    mock_host_connection.device_change_callback(std::move(input_config));

    platform.dispatchable()->dispatch(mir::dispatch::readable);
}

TEST_F(TestNestedInputPlatform, devices_forward_input_events)
{
    auto nested_input_device = capture_input_device(a_mouse);
    NiceMock<mtd::MockInputSink> event_sink;
    mi::DefaultEventBuilder builder(MirInputDeviceId{12}, mir::cookie::Authority::create(), mt::fake_shared(mock_seat));

    ASSERT_THAT(nested_input_device, Ne(nullptr));
    nested_input_device->start(&event_sink, &builder);

    EXPECT_CALL(event_sink, handle_input(
            AllOf(mt::PointerAxisChange(mir_pointer_axis_relative_x, 12),
                  mt::PointerAxisChange(mir_pointer_axis_relative_y, 10),
                  mt::InputDeviceIdMatches(MirInputDeviceId{12})
                  ))
            );

    std::vector<uint8_t> cookie;

    mock_host_connection.event_callback(*mev::make_event(a_mouse.id(), 12ns, cookie, mir_input_event_modifier_none,
                                                         mir_pointer_action_motion, mir_pointer_button_primary, 23, 42,
                                                         0, 0, 12, 10), source_surface);
}

TEST_F(TestNestedInputPlatform, devices_forward_key_events)
{
    auto nested_input_device = capture_input_device(a_keyboard);
    auto const scan_code = 45;
    NiceMock<mtd::MockInputSink> event_sink;
    mi::DefaultEventBuilder builder(MirInputDeviceId{18}, mir::cookie::Authority::create(), mt::fake_shared(mock_seat));

    ASSERT_THAT(nested_input_device, Ne(nullptr));
    nested_input_device->start(&event_sink, &builder);

    EXPECT_CALL(event_sink,
                handle_input(AllOf(
                    mt::KeyDownEvent(), mt::KeyOfScanCode(scan_code), mt::InputDeviceIdMatches(MirInputDeviceId{18}))));
    std::vector<uint8_t> cookie;

    mock_host_connection.event_callback(*mev::make_event(a_keyboard.id(), 141ns, cookie, mir_keyboard_action_down, 0,
                                                         scan_code, mir_input_event_modifier_none), source_surface);
}

TEST_F(TestNestedInputPlatform, replaces_enter_events_as_motion_event)
{
    auto nested_input_device = capture_input_device(a_mouse);
    NiceMock<mtd::MockInputSink> event_sink;
    mi::DefaultEventBuilder builder(MirInputDeviceId{18}, mir::cookie::Authority::create(), mt::fake_shared(mock_seat));

    ASSERT_THAT(nested_input_device, Ne(nullptr));
    nested_input_device->start(&event_sink, &builder);

    EXPECT_CALL(event_sink,
                handle_input(AllOf(
                    mt::PointerEventWithPosition(60.0f, 35.0f), mt::PointerEventWithDiff(60.0f, 35.0f))));
    std::vector<uint8_t> cookie;

    auto event = mev::make_event(a_mouse.id(),
                                 141ns,
                                 cookie,
                                 mir_input_event_modifier_none,
                                 mir_pointer_action_enter,
                                 0,
                                 60.0f, 35.0f,
                                 0, 0,
                                 60.0f, 35.0f);

    mock_host_connection.event_callback(*event, source_surface);
}
