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

#include "src/server/input/default_input_device_hub.h"

#include "mir/test/doubles/mock_input_device.h"
#include "mir/test/doubles/mock_input_device_observer.h"
#include "mir/test/doubles/mock_input_dispatcher.h"
#include "mir/test/doubles/mock_input_region.h"
#include "mir/test/doubles/mock_input_seat.h"
#include "mir/test/doubles/mock_event_sink.h"
#include "mir/test/doubles/mock_key_mapper.h"
#include "mir/test/doubles/stub_cursor_listener.h"
#include "mir/test/doubles/stub_touch_visualizer.h"
#include "mir/test/doubles/triggered_main_loop.h"
#include "mir/test/event_matchers.h"
#include "mir/test/fake_shared.h"

#include "mir/dispatch/action_queue.h"
#include "mir/geometry/rectangles.h"
#include "mir/dispatch/multiplexing_dispatchable.h"
#include "mir/events/event_builders.h"
#include "mir/input/cursor_listener.h"
#include "mir/cookie/authority.h"
#include "mir/graphics/buffer.h"
#include "mir/input/device.h"
#include "mir/input/input_device.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstring>

namespace mi = mir::input;
namespace mt = mir::test;
namespace mtd = mt::doubles;
using namespace std::literals::chrono_literals;
using namespace ::testing;

namespace
{
template<typename T>
T const& const_ref(std::shared_ptr<T> const& ptr)
{
    return *ptr;
}

template<typename T>
T const& const_ref(T const& value)
{
    return value;
}
MATCHER_P(WithName, name,
          std::string(negation?"isn't":"is") +
          " name:" + std::string(name))
{
    return const_ref(arg).name() == name;
}
}

struct InputDeviceHubTest : ::testing::Test
{
    mtd::TriggeredMainLoop observer_loop;
    std::shared_ptr<mir::cookie::Authority> cookie_authority = mir::cookie::Authority::create();
    mir::dispatch::MultiplexingDispatchable multiplexer;
    NiceMock<mtd::MockInputSeat> mock_seat;
    NiceMock<mtd::MockEventSink> mock_sink;
    NiceMock<mtd::MockKeyMapper> mock_key_mapper;
    mi::DefaultInputDeviceHub hub{mt::fake_shared(mock_sink), mt::fake_shared(mock_seat), mt::fake_shared(multiplexer),
                                  mt::fake_shared(observer_loop), cookie_authority, mt::fake_shared(mock_key_mapper)};
    NiceMock<mtd::MockInputDeviceObserver> mock_observer;
    NiceMock<mtd::MockInputDevice> device{"device","dev-1", mi::DeviceCapability::unknown};
    NiceMock<mtd::MockInputDevice> another_device{"another_device","dev-2", mi::DeviceCapability::keyboard};
    NiceMock<mtd::MockInputDevice> third_device{"third_device","dev-3", mi::DeviceCapability::keyboard};

    std::chrono::nanoseconds arbitrary_timestamp;

    void capture_input_sink(NiceMock<mtd::MockInputDevice>& dev, mi::InputSink*& sink, mi::EventBuilder*& builder)
    {
        ON_CALL(dev,start(_,_))
            .WillByDefault(Invoke([&sink,&builder](mi::InputSink* input_sink, mi::EventBuilder* event_builder)
                                  {
                                      sink = input_sink;
                                      builder = event_builder;
                                  }
                                 ));
    }
};

TEST_F(InputDeviceHubTest, input_device_hub_starts_device)
{
    EXPECT_CALL(device,start(_,_));

    hub.add_device(mt::fake_shared(device));
}

TEST_F(InputDeviceHubTest, input_device_hub_stops_device_on_removal)
{
    EXPECT_CALL(device,stop());

    hub.add_device(mt::fake_shared(device));
    hub.remove_device(mt::fake_shared(device));
}

TEST_F(InputDeviceHubTest, input_device_hub_ignores_removal_of_unknown_devices)
{
    EXPECT_CALL(device,start(_,_)).Times(0);
    EXPECT_CALL(device,stop()).Times(0);

    EXPECT_THROW(hub.remove_device(mt::fake_shared(device));, std::logic_error);
}

TEST_F(InputDeviceHubTest, input_device_hub_start_stop_happens_in_order)
{

    InSequence seq;
    EXPECT_CALL(device, start(_,_));
    EXPECT_CALL(another_device, start(_,_));
    EXPECT_CALL(third_device, start(_,_));
    EXPECT_CALL(another_device, stop());
    EXPECT_CALL(device, stop());
    EXPECT_CALL(third_device, stop());

    hub.add_device(mt::fake_shared(device));
    hub.add_device(mt::fake_shared(another_device));
    hub.add_device(mt::fake_shared(third_device));
    hub.remove_device(mt::fake_shared(another_device));
    hub.remove_device(mt::fake_shared(device));
    hub.remove_device(mt::fake_shared(third_device));
}

TEST_F(InputDeviceHubTest, observers_receive_devices_on_add)
{
    std::shared_ptr<mi::Device> handle_1, handle_2;

    InSequence seq;
    EXPECT_CALL(mock_observer,device_added(WithName("device"))).WillOnce(SaveArg<0>(&handle_1));
    EXPECT_CALL(mock_observer,device_added(WithName("another_device"))).WillOnce(SaveArg<0>(&handle_2));
    EXPECT_CALL(mock_observer,changes_complete());

    hub.add_device(mt::fake_shared(device));
    hub.add_device(mt::fake_shared(another_device));
    hub.add_observer(mt::fake_shared(mock_observer));

    observer_loop.trigger_server_actions();

    EXPECT_THAT(handle_1,Ne(handle_2));
    EXPECT_THAT(handle_1->unique_id(),Ne(handle_2->unique_id()));
}

TEST_F(InputDeviceHubTest, seat_receives_devices)
{
    InSequence seq;
    EXPECT_CALL(mock_seat, add_device(WithName("device")));
    EXPECT_CALL(mock_seat, add_device(WithName("another_device")));
    EXPECT_CALL(mock_seat, remove_device(WithName("device")));

    hub.add_device(mt::fake_shared(device));
    hub.add_device(mt::fake_shared(another_device));
    hub.remove_device(mt::fake_shared(device));
}

TEST_F(InputDeviceHubTest, throws_on_duplicate_add)
{
    hub.add_device(mt::fake_shared(device));
    EXPECT_THROW(hub.add_device(mt::fake_shared(device)), std::logic_error);
}

TEST_F(InputDeviceHubTest, throws_on_spurious_remove)
{
    hub.add_device(mt::fake_shared(device));
    hub.remove_device(mt::fake_shared(device));
    EXPECT_THROW(hub.remove_device(mt::fake_shared(device)), std::logic_error);
}

TEST_F(InputDeviceHubTest, throws_on_invalid_handles)
{
    EXPECT_THROW(hub.add_device(std::shared_ptr<mi::InputDevice>()), std::logic_error);
    EXPECT_THROW(hub.remove_device(std::shared_ptr<mi::InputDevice>()), std::logic_error);
}

TEST_F(InputDeviceHubTest, observers_receive_device_changes)
{
    InSequence seq;
    EXPECT_CALL(mock_observer, changes_complete());
    EXPECT_CALL(mock_observer, device_added(WithName("device")));
    EXPECT_CALL(mock_observer, changes_complete());
    EXPECT_CALL(mock_observer, device_removed(WithName("device")));
    EXPECT_CALL(mock_observer, changes_complete());

    hub.add_observer(mt::fake_shared(mock_observer));
    hub.add_device(mt::fake_shared(device));
    hub.remove_device(mt::fake_shared(device));

    observer_loop.trigger_server_actions();
}

