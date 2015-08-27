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

#include "mir/test/doubles/triggered_main_loop.h"
#include "mir/test/doubles/mock_input_dispatcher.h"
#include "mir/test/doubles/mock_input_region.h"
#include "mir/test/fake_shared.h"
#include "mir/test/event_matchers.h"

#include "mir/input/input_device.h"
#include "mir/input/input_device_info.h"
#include "mir/input/touch_visualizer.h"
#include "mir/input/input_device_observer.h"
#include "mir/dispatch/multiplexing_dispatchable.h"
#include "mir/dispatch/action_queue.h"
#include "mir/events/event_builders.h"
#include "mir/input/cursor_listener.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstring>

namespace mi = mir::input;
namespace mt = mir::test;
namespace mtd = mt::doubles;
namespace geom = mir::geometry;
using namespace std::literals::chrono_literals;

namespace mir
{
namespace input
{
std::ostream& operator<<(std::ostream& out, InputDeviceInfo const& info)
{
    return out << info.id << ' ' << info.name << ' ' << info.unique_id;
}

bool operator==(mir::input::TouchVisualizer::Spot const& lhs, mir::input::TouchVisualizer::Spot const& rhs)
{
    return lhs.touch_location == rhs.touch_location && lhs.pressure == rhs.pressure;
}
}
}

struct MockTouchVisualizer : public mi::TouchVisualizer
{
    MOCK_METHOD1(visualize_touches, void(std::vector<mi::TouchVisualizer::Spot> const&));
    MOCK_METHOD0(enable, void());
    MOCK_METHOD0(disable, void());
};

struct MockInputDeviceObserver : public mi::InputDeviceObserver
{
    MOCK_METHOD1(device_added, void (mi::InputDeviceInfo const& device));
    MOCK_METHOD1(device_changed, void(mi::InputDeviceInfo const& device));
    MOCK_METHOD1(device_removed, void(mi::InputDeviceInfo const& device));
    MOCK_METHOD0(changes_complete, void());
};

struct MockCursorListener : public mi::CursorListener
{
    MOCK_METHOD2(cursor_moved_to, void(float, float));

    ~MockCursorListener() noexcept {}
};

struct MockInputDevice : public mi::InputDevice
{
    mir::dispatch::ActionQueue queue;
    std::shared_ptr<mir::dispatch::Dispatchable> dispatchable() { return mt::fake_shared(queue);}
    MOCK_METHOD2(start, void(mi::InputSink* destination, mi::EventBuilder* builder));
    MOCK_METHOD0(stop, void());
    MOCK_METHOD0(get_device_info, mi::InputDeviceInfo());
};

template<typename Type>
using Nice = ::testing::NiceMock<Type>;

struct InputDeviceHubTest : ::testing::Test
{
    mtd::TriggeredMainLoop observer_loop;
    Nice<mtd::MockInputDispatcher> mock_dispatcher;
    Nice<mtd::MockInputRegion> mock_region;
    Nice<MockCursorListener> mock_cursor_listener;
    Nice<MockTouchVisualizer> mock_visualizer;
    mir::dispatch::MultiplexingDispatchable multiplexer;
    mi::DefaultInputDeviceHub hub{mt::fake_shared(mock_dispatcher), mt::fake_shared(multiplexer),
                                  mt::fake_shared(observer_loop), mt::fake_shared(mock_visualizer),
                                  mt::fake_shared(mock_cursor_listener), mt::fake_shared(mock_region)};
    Nice<MockInputDeviceObserver> mock_observer;
    Nice<MockInputDevice> device;
    Nice<MockInputDevice> another_device;
    Nice<MockInputDevice> third_device;

    std::chrono::nanoseconds arbitrary_timestamp;

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

    void capture_input_sink(Nice<MockInputDevice>& dev, mi::InputSink*& sink, mi::EventBuilder*& builder)
    {
        using namespace ::testing;
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
    using namespace ::testing;

    EXPECT_CALL(device,start(_,_));

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

    EXPECT_CALL(device,start(_,_)).Times(0);
    EXPECT_CALL(device,stop()).Times(0);

    EXPECT_THROW(hub.remove_device(mt::fake_shared(device));, std::logic_error);
}

TEST_F(InputDeviceHubTest, input_device_hub_start_stop_happens_in_order)
{
    using namespace ::testing;

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

    mi::InputSink* sink;
    mi::EventBuilder* builder;
    mi::InputDeviceInfo info;

    capture_input_sink(device, sink, builder);

    EXPECT_CALL(mock_observer,device_added(_))
        .WillOnce(SaveArg<0>(&info));

    hub.add_observer(mt::fake_shared(mock_observer));
    hub.add_device(mt::fake_shared(device));

    observer_loop.trigger_server_actions();

    auto event = builder->key_event(arbitrary_timestamp, mir_keyboard_action_down, 0,
                                    KEY_A, mir_input_event_modifier_none);

    EXPECT_CALL(mock_dispatcher, dispatch(AllOf(mt::InputDeviceIdMatches(info.id), mt::MirKeyEventMatches(event.get()))));

    sink->handle_input(*event);
}

TEST_F(InputDeviceHubTest, forwards_touch_spots_to_visualizer)
{
    using namespace ::testing;
    mi::InputSink* sink;
    mi::InputDeviceInfo info;
    mi::EventBuilder* builder;

    capture_input_sink(device, sink, builder);

    hub.add_device(mt::fake_shared(device));

    observer_loop.trigger_server_actions();

    auto touch_event_1 = builder->touch_event(arbitrary_timestamp, mir_input_event_modifier_none);
    builder->add_touch(*touch_event_1, 0, mir_touch_action_down, mir_touch_tooltype_finger, 21.0f, 34.0f, 50.0f, 15.0f,
                       5.0f, 4.0f);

    auto touch_event_2 = builder->touch_event(arbitrary_timestamp, mir_input_event_modifier_none);
    builder->add_touch(*touch_event_2, 0, mir_touch_action_change, mir_touch_tooltype_finger, 24.0f, 34.0f, 50.0f,
                       15.0f, 5.0f, 4.0f);
    builder->add_touch(*touch_event_2, 1, mir_touch_action_down, mir_touch_tooltype_finger, 60.0f, 34.0f, 50.0f, 15.0f,
                       5.0f, 4.0f);

    auto touch_event_3 = builder->touch_event(arbitrary_timestamp, mir_input_event_modifier_none);
    builder->add_touch(*touch_event_3, 0, mir_touch_action_up, mir_touch_tooltype_finger, 24.0f, 34.0f, 50.0f, 15.0f,
                       5.0f, 4.0f);
    builder->add_touch(*touch_event_3, 1, mir_touch_action_change, mir_touch_tooltype_finger, 70.0f, 30.0f, 50.0f,
                       15.0f, 5.0f, 4.0f);

    auto touch_event_4 = builder->touch_event(arbitrary_timestamp, mir_input_event_modifier_none);
    builder->add_touch(*touch_event_4, 1, mir_touch_action_up, mir_touch_tooltype_finger, 70.0f, 35.0f, 50.0f, 15.0f,
                       5.0f, 4.0f);


    using Spot = mi::TouchVisualizer::Spot;

    InSequence seq;
    EXPECT_CALL(mock_visualizer, visualize_touches(ElementsAreArray({Spot{{21,34}, 50}})));
    EXPECT_CALL(mock_visualizer, visualize_touches(ElementsAreArray({Spot{{24,34}, 50}, Spot{{60,34}, 50}})));
    EXPECT_CALL(mock_visualizer, visualize_touches(ElementsAreArray({Spot{{70,30}, 50}})));
    EXPECT_CALL(mock_visualizer, visualize_touches(ElementsAreArray(std::vector<Spot>())));

    sink->handle_input(*touch_event_1);
    sink->handle_input(*touch_event_2);
    sink->handle_input(*touch_event_3);
    sink->handle_input(*touch_event_4);
}


TEST_F(InputDeviceHubTest, tracks_pointer_position)
{
    geom::Point first{10,10}, second{20,20}, third{10,30};
    EXPECT_CALL(mock_region,confine(first));
    EXPECT_CALL(mock_region,confine(second));
    EXPECT_CALL(mock_region,confine(third));

    mi::InputSink* sink;
    mi::EventBuilder* builder;
    capture_input_sink(device, sink, builder);

    hub.add_device(mt::fake_shared(device));

    geom::Point pos = first;
    sink->confine_pointer(pos);
    pos = second;
    sink->confine_pointer(pos);
    pos = third;
    sink->confine_pointer(pos);
}

TEST_F(InputDeviceHubTest, confines_pointer_movement)
{
    using namespace ::testing;
    geom::Point confined_pos{10, 18};

    ON_CALL(mock_region,confine(_))
        .WillByDefault(SetArgReferee<0>(confined_pos));

    mi::InputSink* sink;
    mi::EventBuilder* builder;
    capture_input_sink(device, sink, builder);
    hub.add_device(mt::fake_shared(device));

    geom::Point pos1{10,20};
    sink->confine_pointer(pos1);

    geom::Point pos2{20,30};
    sink->confine_pointer(pos2);

    EXPECT_THAT(pos1, Eq(confined_pos));
    EXPECT_THAT(pos2, Eq(confined_pos));
}

TEST_F(InputDeviceHubTest, forwards_pointer_updates_to_cursor_listener)
{
    using namespace ::testing;

    auto x = 12.2f, y = 14.3f;
    auto mac = 0;

    auto event = mir::events::make_event(0, 0ns, mac, mir_input_event_modifier_none, mir_pointer_action_motion, 0,
        x, y, 0.0f, 0.0f);

    EXPECT_CALL(mock_cursor_listener, cursor_moved_to(x, y)).Times(1);

    mi::InputSink* sink;
    mi::EventBuilder* builder;
    capture_input_sink(device, sink, builder);
    hub.add_device(mt::fake_shared(device));

    sink->handle_input(*event);
}
