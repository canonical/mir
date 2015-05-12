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

#include "src/server/input/default_input_device_hub.h"

#include "mir_test_doubles/triggered_main_loop.h"
#include "mir_test_doubles/mock_input_dispatcher.h"
#include "mir_test/fake_shared.h"

#include "mir/input/input_device.h"
#include "mir/input/input_device_info.h"
#include "mir/events/event_private.h"
#include "mir/dispatch/multiplexing_dispatchable.h"
#include "mir/dispatch/action_queue.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstring>

namespace mi = mir::input;
namespace mt = mir::test;
namespace mtd = mt::doubles;

namespace mir
{
namespace input
{
std::ostream& operator<<(std::ostream& out, InputDeviceInfo const& info)
{
    return out << info.id << ' ' << info.name << ' ' << info.unique_id;
}
}
}

struct MockInputDeviceObserver : public mi::InputDeviceObserver
{
    MOCK_METHOD1(device_added, void (mi::InputDeviceInfo const& device));
    MOCK_METHOD1(device_changed, void(mi::InputDeviceInfo const& device));
    MOCK_METHOD1(device_removed, void(mi::InputDeviceInfo const& device));
    MOCK_METHOD0(changes_complete, void());
};

struct MockInputDevice : public mi::InputDevice
{
    mir::dispatch::ActionQueue queue;
    std::shared_ptr<mir::dispatch::Dispatchable> dispatchable() { return mt::fake_shared(queue);}
    MOCK_METHOD1(start, void(mi::InputSink* destination));
    MOCK_METHOD0(stop, void());
    MOCK_METHOD0(get_device_info, mi::InputDeviceInfo());
};

template<typename Type>
using Nice = ::testing::NiceMock<Type>;

struct InputDeviceHubTest : ::testing::Test
{
    mtd::TriggeredMainLoop observer_loop;
    Nice<mtd::MockInputDispatcher> mock_dispatcher;
    mir::dispatch::MultiplexingDispatchable multiplexer;
    mi::DefaultInputDeviceHub hub{mt::fake_shared(mock_dispatcher), mt::fake_shared(multiplexer),
                                  mt::fake_shared(observer_loop)};
    Nice<MockInputDeviceObserver> mock_observer;
    Nice<MockInputDevice> device;
    Nice<MockInputDevice> another_device;
    Nice<MockInputDevice> third_device;

    InputDeviceHubTest()
    {
        using namespace testing;
        ON_CALL(device,get_device_info())
            .WillByDefault(Return(mi::InputDeviceInfo{0,"device","dev-1", mi::DeviceCapability::unknown}));

        ON_CALL(another_device,get_device_info())
            .WillByDefault(Return(mi::InputDeviceInfo{0,"another_device","dev-2", mi::DeviceCapability::keyboard}));

        ON_CALL(third_device,get_device_info())
            .WillByDefault(Return(mi::InputDeviceInfo{0,"third_device","dev-3", mi::DeviceCapability::keyboard}));
    }
};

TEST_F(InputDeviceHubTest, input_device_hub_starts_device)
{
    using namespace ::testing;

    EXPECT_CALL(device,start(_));

    hub.add_device(mt::fake_shared(device));
}

TEST_F(InputDeviceHubTest, input_device_hub_stops_device_on_removal)
{
    using namespace ::testing;

    EXPECT_CALL(device,stop());

    hub.add_device(mt::fake_shared(device));
    hub.remove_device(mt::fake_shared(device));
}

TEST_F(InputDeviceHubTest, input_device_hub_ignores_removal_of_unknown_devices)
{
    using namespace ::testing;

    EXPECT_CALL(device,start(_)).Times(0);
    EXPECT_CALL(device,stop()).Times(0);

    EXPECT_THROW(hub.remove_device(mt::fake_shared(device));, std::logic_error);
}

TEST_F(InputDeviceHubTest, input_device_hub_start_stop_happens_in_order)
{
    using namespace ::testing;

    InSequence seq;
    EXPECT_CALL(device, start(_));
    EXPECT_CALL(another_device, start(_));
    EXPECT_CALL(third_device, start(_));
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

MATCHER_P(WithName, name,
          std::string(negation?"isn't":"is") +
          " name:" + std::string(name))
{
    return arg.name == name;
}

TEST_F(InputDeviceHubTest, observers_receive_devices_on_add)
{
    using namespace ::testing;

    mi::InputDeviceInfo info1, info2;

    InSequence seq;
    EXPECT_CALL(mock_observer,device_added(WithName("device"))).WillOnce(SaveArg<0>(&info1));
    EXPECT_CALL(mock_observer,device_added(WithName("another_device"))).WillOnce(SaveArg<0>(&info2));
    EXPECT_CALL(mock_observer,changes_complete());

    hub.add_device(mt::fake_shared(device));
    hub.add_device(mt::fake_shared(another_device));
    hub.add_observer(mt::fake_shared(mock_observer));

    observer_loop.trigger_server_actions();

    EXPECT_THAT(info1.id,Ne(info2.id));
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
    using namespace ::testing;

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

TEST_F(InputDeviceHubTest, input_sink_posts_events_to_input_dispatcher)
{
    using namespace ::testing;
    MirEvent event;
    std::memset(&event, 0, sizeof event);
    event.type = mir_event_type_key;
    mi::InputSink* sink;
    mi::InputDeviceInfo info;
    MirEvent dispatched_event;

    EXPECT_CALL(device,start(_))
        .WillOnce(Invoke([&sink](mi::InputSink* input_sink)
                         {
                             sink = input_sink;
                         }
                        ));

    EXPECT_CALL(mock_observer,device_added(_))
        .WillOnce(SaveArg<0>(&info));
    EXPECT_CALL(mock_dispatcher,dispatch(_))
        .WillOnce(SaveArg<0>(&dispatched_event));

    hub.add_observer(mt::fake_shared(mock_observer));
    hub.add_device(mt::fake_shared(device));

    observer_loop.trigger_server_actions();
    sink->handle_input(event);

    EXPECT_THAT(dispatched_event.key.device_id, Eq(info.id));
    EXPECT_THAT(dispatched_event.type, Eq(event.type));
}
