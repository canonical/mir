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
#include "mir/input/pointer_settings.h"
#include "mir/input/touchpad_settings.h"
#include "mir/input/device.h"
#include "mir/input/touch_visualizer.h"
#include "mir/input/pointer_configuration.h"
#include "mir/input/touchpad_configuration.h"
#include "mir/input/input_device_observer.h"
#include "mir/dispatch/multiplexing_dispatchable.h"
#include "mir/dispatch/action_queue.h"
#include "mir/events/event_builders.h"
#include "mir/input/cursor_listener.h"
#include "mir/cookie_authority.h"

#include "mir/input/input_device_info.h" // needed for fake device setup

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstring>

namespace mi = mir::input;
namespace mt = mir::test;
namespace mtd = mt::doubles;
namespace geom = mir::geometry;
using namespace std::literals::chrono_literals;
using namespace ::testing;

namespace mir
{
namespace input
{
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
    MOCK_METHOD1(device_added, void(std::shared_ptr<mi::Device> const& device));
    MOCK_METHOD1(device_changed, void(std::shared_ptr<mi::Device> const& device));
    MOCK_METHOD1(device_removed, void(std::shared_ptr<mi::Device> const& device));
    MOCK_METHOD0(changes_complete, void());
};

struct MockCursorListener : public mi::CursorListener
{
    MOCK_METHOD2(cursor_moved_to, void(float, float));

    ~MockCursorListener() noexcept {}
};

struct MockInputDevice : public mi::InputDevice
{
    MOCK_METHOD2(start, void(mi::InputSink* destination, mi::EventBuilder* builder));
    MOCK_METHOD0(stop, void());
    MOCK_METHOD0(get_device_info, mi::InputDeviceInfo());
    MOCK_CONST_METHOD0(get_pointer_settings, mir::optional_value<mi::PointerSettings>());
    MOCK_METHOD1(apply_settings, void(mi::PointerSettings const&));
    MOCK_CONST_METHOD0(get_touchpad_settings, mir::optional_value<mi::TouchpadSettings>());
    MOCK_METHOD1(apply_settings, void(mi::TouchpadSettings const&));
};

template<typename Type>
using Nice = ::testing::NiceMock<Type>;

struct InputDeviceHubTest : ::testing::Test
{
    mtd::TriggeredMainLoop observer_loop;
    Nice<mtd::MockInputDispatcher> mock_dispatcher;
    Nice<mtd::MockInputRegion> mock_region;
    std::shared_ptr<mir::cookie::CookieAuthority> cookie_authority = mir::cookie::CookieAuthority::create_keeping_secret();
    Nice<MockCursorListener> mock_cursor_listener;
    Nice<MockTouchVisualizer> mock_visualizer;
    mir::dispatch::MultiplexingDispatchable multiplexer;
    mi::DefaultInputDeviceHub hub{mt::fake_shared(mock_dispatcher), mt::fake_shared(multiplexer),
                                  mt::fake_shared(observer_loop), mt::fake_shared(mock_visualizer),
                                  mt::fake_shared(mock_cursor_listener), mt::fake_shared(mock_region), cookie_authority};
    Nice<MockInputDeviceObserver> mock_observer;
    Nice<MockInputDevice> device;
    Nice<MockInputDevice> another_device;
    Nice<MockInputDevice> third_device;
    Nice<MockInputDevice> touchpad;

    std::chrono::nanoseconds arbitrary_timestamp;

    InputDeviceHubTest()
    {
        ON_CALL(device, get_device_info())
            .WillByDefault(Return(mi::InputDeviceInfo{"device","dev-1", mi::DeviceCapability::unknown}));
        ON_CALL(device, get_pointer_settings())
            .WillByDefault(Return(mir::optional_value<mi::PointerSettings>()));
        ON_CALL(device, get_touchpad_settings())
            .WillByDefault(Return(mir::optional_value<mi::TouchpadSettings>()));

        ON_CALL(another_device, get_device_info())
            .WillByDefault(Return(mi::InputDeviceInfo{"another_device","dev-2", mi::DeviceCapability::keyboard}));
        ON_CALL(another_device, get_pointer_settings())
            .WillByDefault(Return(mir::optional_value<mi::PointerSettings>()));
        ON_CALL(another_device, get_touchpad_settings())
            .WillByDefault(Return(mir::optional_value<mi::TouchpadSettings>()));

        ON_CALL(third_device,get_device_info())
            .WillByDefault(Return(mi::InputDeviceInfo{"third_device","dev-3", mi::DeviceCapability::keyboard}));
        ON_CALL(third_device, get_pointer_settings())
            .WillByDefault(Return(mir::optional_value<mi::PointerSettings>()));
        ON_CALL(third_device, get_touchpad_settings())
            .WillByDefault(Return(mir::optional_value<mi::TouchpadSettings>()));

        ON_CALL(touchpad, get_device_info())
            .WillByDefault(Return(mi::InputDeviceInfo{"touchpad", "dev-4", mi::DeviceCapability::touchpad|mi::DeviceCapability::pointer}));
        ON_CALL(touchpad, get_pointer_settings())
            .WillByDefault(Return(mi::PointerSettings()));
        ON_CALL(touchpad, get_touchpad_settings())
            .WillByDefault(Return(mi::TouchpadSettings()));
    }

    void capture_input_sink(Nice<MockInputDevice>& dev, mi::InputSink*& sink, mi::EventBuilder*& builder)
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

MATCHER_P(WithName, name,
          std::string(negation?"isn't":"is") +
          " name:" + std::string(name))
{
    return arg->name() == name;
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

TEST_F(InputDeviceHubTest, input_sink_posts_events_to_input_dispatcher)
{
    mi::InputSink* sink;
    mi::EventBuilder* builder;
    std::shared_ptr<mi::Device> handle;

    capture_input_sink(device, sink, builder);

    EXPECT_CALL(mock_observer,device_added(_))
        .WillOnce(SaveArg<0>(&handle));

    hub.add_observer(mt::fake_shared(mock_observer));
    hub.add_device(mt::fake_shared(device));

    observer_loop.trigger_server_actions();

    auto event = builder->key_event(arbitrary_timestamp, mir_keyboard_action_down, 0,
                                    KEY_A);

    EXPECT_CALL(mock_dispatcher, dispatch(AllOf(mt::InputDeviceIdMatches(handle->id()), mt::MirKeyboardEventMatches(event.get()))));

    sink->handle_input(*event);
}

TEST_F(InputDeviceHubTest, forwards_touch_spots_to_visualizer)
{
    mi::InputSink* sink;
    mi::EventBuilder* builder;

    capture_input_sink(device, sink, builder);

    hub.add_device(mt::fake_shared(device));

    observer_loop.trigger_server_actions();

    auto touch_event_1 = builder->touch_event(arbitrary_timestamp);
    builder->add_touch(*touch_event_1, 0, mir_touch_action_down, mir_touch_tooltype_finger, 21.0f, 34.0f, 50.0f, 15.0f,
                       5.0f, 4.0f);

    auto touch_event_2 = builder->touch_event(arbitrary_timestamp);
    builder->add_touch(*touch_event_2, 0, mir_touch_action_change, mir_touch_tooltype_finger, 24.0f, 34.0f, 50.0f,
                       15.0f, 5.0f, 4.0f);
    builder->add_touch(*touch_event_2, 1, mir_touch_action_down, mir_touch_tooltype_finger, 60.0f, 34.0f, 50.0f, 15.0f,
                       5.0f, 4.0f);

    auto touch_event_3 = builder->touch_event(arbitrary_timestamp);
    builder->add_touch(*touch_event_3, 0, mir_touch_action_up, mir_touch_tooltype_finger, 24.0f, 34.0f, 50.0f, 15.0f,
                       5.0f, 4.0f);
    builder->add_touch(*touch_event_3, 1, mir_touch_action_change, mir_touch_tooltype_finger, 70.0f, 30.0f, 50.0f,
                       15.0f, 5.0f, 4.0f);

    auto touch_event_4 = builder->touch_event(arbitrary_timestamp);
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
    EXPECT_CALL(mock_region, confine(first));
    EXPECT_CALL(mock_region, confine(second));
    EXPECT_CALL(mock_region, confine(third));

    mi::InputSink* sink;
    mi::EventBuilder* builder;
    capture_input_sink(device, sink, builder);

    hub.add_device(mt::fake_shared(device));
    observer_loop.trigger_server_actions();
    sink->handle_input(
        *builder->pointer_event(arbitrary_timestamp, mir_pointer_action_motion, 0, 0.0f, 0.0f, 10.0f, 10.0f));
    sink->handle_input(
        *builder->pointer_event(arbitrary_timestamp, mir_pointer_action_motion, 0, 0.0f, 0.0f, 10.0f, 10.0f));
    sink->handle_input(
        *builder->pointer_event(arbitrary_timestamp, mir_pointer_action_motion, 0, 0.0f, 0.0f, -10.0f, 10.0f));
}

TEST_F(InputDeviceHubTest, confines_pointer_movement)
{
    geom::Point confined_pos{10, 18};

    ON_CALL(mock_region,confine(_))
        .WillByDefault(SetArgReferee<0>(confined_pos));
    EXPECT_CALL(mock_cursor_listener, cursor_moved_to(confined_pos.x.as_int(), confined_pos.y.as_int())).Times(2);

    mi::InputSink* sink;
    mi::EventBuilder* builder;
    capture_input_sink(device, sink, builder);
    hub.add_device(mt::fake_shared(device));
    observer_loop.trigger_server_actions();

    sink->handle_input(
        *builder->pointer_event(arbitrary_timestamp, mir_pointer_action_motion, 0, 0.0f, 0.0f, 10.0f, 20.0f));
    sink->handle_input(
        *builder->pointer_event(arbitrary_timestamp, mir_pointer_action_motion, 0, 0.0f, 0.0f, 10.0f, 10.0f));
}

TEST_F(InputDeviceHubTest, forwards_pointer_updates_to_cursor_listener)
{
    using namespace ::testing;

    auto move_x = 12.0f, move_y = 14.0f;

    mi::InputSink* sink;
    mi::EventBuilder* builder;
    capture_input_sink(device, sink, builder);
    hub.add_device(mt::fake_shared(device));

    auto event = builder->pointer_event(0ns, mir_pointer_action_motion, 0, 0.0f, 0.0f, move_x, move_y);

    EXPECT_CALL(mock_cursor_listener, cursor_moved_to(move_x, move_y)).Times(1);

    sink->handle_input(*event);
}

TEST_F(InputDeviceHubTest, forwards_pointer_settings_to_input_device)
{
    std::shared_ptr<mi::Device> dev;
    ON_CALL(mock_observer, device_added(_)).WillByDefault(SaveArg<0>(&dev));

    hub.add_observer(mt::fake_shared(mock_observer));
    hub.add_device(mt::fake_shared(touchpad));
    observer_loop.trigger_server_actions();

    EXPECT_CALL(touchpad, apply_settings(Matcher<mi::PointerSettings const&>(_)));

    auto conf = dev->pointer_configuration();
    dev->apply_pointer_configuration(conf.value());
    multiplexer.dispatch(mir::dispatch::FdEvent::readable);
}

TEST_F(InputDeviceHubTest, forwards_touchpad_settings_to_input_device)
{
    std::shared_ptr<mi::Device> dev;
    ON_CALL(mock_observer, device_added(_)).WillByDefault(SaveArg<0>(&dev));

    hub.add_observer(mt::fake_shared(mock_observer));
    hub.add_device(mt::fake_shared(touchpad));
    observer_loop.trigger_server_actions();

    EXPECT_CALL(touchpad, apply_settings(Matcher<mi::TouchpadSettings const&>(_)));

    auto conf = dev->touchpad_configuration();
    dev->apply_touchpad_configuration(conf.value());
    multiplexer.dispatch(mir::dispatch::FdEvent::readable);
}

TEST_F(InputDeviceHubTest, input_sink_tracks_modifier)
{
    using namespace ::testing;

    mi::InputSink* key_board_sink;
    mi::EventBuilder* key_event_builder;
    std::shared_ptr<mi::Device> key_handle;

    capture_input_sink(device,  key_board_sink, key_event_builder);

    InSequence seq;
    EXPECT_CALL(mock_observer,device_added(_))
        .WillOnce(SaveArg<0>(&key_handle));

    hub.add_observer(mt::fake_shared(mock_observer));
    hub.add_device(mt::fake_shared(device));

    observer_loop.trigger_server_actions();

    const MirInputEventModifiers shift_left = mir_input_event_modifier_shift_left | mir_input_event_modifier_shift;
    auto shift_down =
        key_event_builder->key_event(arbitrary_timestamp, mir_keyboard_action_down, 0, KEY_LEFTSHIFT);
    auto shift_up =
        key_event_builder->key_event(arbitrary_timestamp, mir_keyboard_action_up, 0, KEY_LEFTSHIFT);

    EXPECT_CALL(mock_dispatcher, dispatch(mt::KeyWithModifiers(shift_left)));
    EXPECT_CALL(mock_dispatcher, dispatch(mt::KeyWithModifiers(mir_input_event_modifier_none)));

    key_board_sink->handle_input(*shift_down);
    key_board_sink->handle_input(*shift_up);
}

TEST_F(InputDeviceHubTest, input_sink_unifies_modifier_state_accross_devices)
{
    using namespace ::testing;

    mi::InputSink* mouse_sink;
    mi::EventBuilder* mouse_event_builder;
    mi::InputSink* key_board_sink;
    mi::EventBuilder* key_event_builder;
    std::shared_ptr<mi::Device> mouse_handle;
    std::shared_ptr<mi::Device> key_handle;

    capture_input_sink(device, mouse_sink, mouse_event_builder);
    capture_input_sink(another_device, key_board_sink, key_event_builder);

    InSequence seq;
    EXPECT_CALL(mock_observer,device_added(_))
        .WillOnce(SaveArg<0>(&mouse_handle));
    EXPECT_CALL(mock_observer,device_added(_))
        .WillOnce(SaveArg<0>(&key_handle));

    hub.add_observer(mt::fake_shared(mock_observer));
    hub.add_device(mt::fake_shared(device));
    hub.add_device(mt::fake_shared(another_device));

    observer_loop.trigger_server_actions();

    const MirInputEventModifiers r_alt_modifier = mir_input_event_modifier_alt_right | mir_input_event_modifier_alt;
    auto key =
        key_event_builder->key_event(arbitrary_timestamp, mir_keyboard_action_down, 0, KEY_RIGHTALT);
    auto motion =
        mouse_event_builder->pointer_event(arbitrary_timestamp, mir_pointer_action_motion, 0, 0, 0, 12, 40);

    EXPECT_CALL(mock_dispatcher, dispatch(mt::KeyWithModifiers(r_alt_modifier)));
    EXPECT_CALL(mock_dispatcher, dispatch(mt::PointerEventWithModifiers(r_alt_modifier)));

    key_board_sink->handle_input(*key);
    mouse_sink->handle_input(*motion);

    EXPECT_THAT(key_handle->id(), Ne(mouse_handle->id()));
}

TEST_F(InputDeviceHubTest, input_sink_reduces_modifier_state_accross_devices)
{
    using namespace ::testing;

    mi::InputSink* mouse_sink;
    mi::EventBuilder* mouse_event_builder;
    mi::InputSink* key_board_sink_1;
    mi::EventBuilder* key_event_builder_1;
    mi::InputSink* key_board_sink_2;
    mi::EventBuilder* key_event_builder_2;
    std::shared_ptr<mi::Device> mouse_handle;
    std::shared_ptr<mi::Device> key_handle_1;
    std::shared_ptr<mi::Device> key_handle_2;

    capture_input_sink(device, mouse_sink, mouse_event_builder);
    capture_input_sink(another_device, key_board_sink_1, key_event_builder_1);
    capture_input_sink(third_device, key_board_sink_2, key_event_builder_2);

    InSequence seq;
    EXPECT_CALL(mock_observer, device_added(_))
        .WillOnce(SaveArg<0>(&mouse_handle));
    EXPECT_CALL(mock_observer, device_added(_))
        .WillOnce(SaveArg<0>(&key_handle_1));
    EXPECT_CALL(mock_observer, device_added(_))
        .WillOnce(SaveArg<0>(&key_handle_2));

    hub.add_observer(mt::fake_shared(mock_observer));
    hub.add_device(mt::fake_shared(device));
    hub.add_device(mt::fake_shared(another_device));
    hub.add_device(mt::fake_shared(third_device));

    const MirInputEventModifiers r_alt_modifier = mir_input_event_modifier_alt_right | mir_input_event_modifier_alt;
    const MirInputEventModifiers l_ctrl_modifier = mir_input_event_modifier_ctrl_left | mir_input_event_modifier_ctrl;
    const MirInputEventModifiers combined_modifier = r_alt_modifier | l_ctrl_modifier;

    observer_loop.trigger_server_actions();

    auto alt_down = key_event_builder_1->key_event(arbitrary_timestamp, mir_keyboard_action_down, 0, KEY_RIGHTALT);
    auto ctrl_down = key_event_builder_2->key_event(arbitrary_timestamp, mir_keyboard_action_down, 0, KEY_LEFTCTRL);
    auto ctrl_up = key_event_builder_2->key_event(arbitrary_timestamp, mir_keyboard_action_up, 0, KEY_LEFTCTRL);

    auto motion_1 =
        mouse_event_builder->pointer_event(arbitrary_timestamp, mir_pointer_action_motion, 0, 0, 0, 12, 40);
    auto motion_2 =
        mouse_event_builder->pointer_event(arbitrary_timestamp, mir_pointer_action_motion, 0, 0, 0, 18, 10);

    EXPECT_CALL(mock_dispatcher, dispatch(mt::KeyWithModifiers(r_alt_modifier)));
    EXPECT_CALL(mock_dispatcher, dispatch(mt::KeyWithModifiers(l_ctrl_modifier)));
    EXPECT_CALL(mock_dispatcher, dispatch(mt::PointerEventWithModifiers(combined_modifier)));
    EXPECT_CALL(mock_dispatcher, dispatch(mt::KeyWithModifiers(mir_input_event_modifier_none)));
    EXPECT_CALL(mock_dispatcher, dispatch(mt::PointerEventWithModifiers(r_alt_modifier)));

    key_board_sink_1->handle_input(*alt_down);
    key_board_sink_2->handle_input(*ctrl_down);
    mouse_sink->handle_input(*motion_1);
    key_board_sink_2->handle_input(*ctrl_up);
    mouse_sink->handle_input(*motion_2);

    EXPECT_THAT(key_handle_1->id(), Ne(key_handle_2->id()));
}

TEST_F(InputDeviceHubTest, tracks_a_single_cursor_position_from_multiple_pointing_devices)
{
    using namespace ::testing;

    mi::InputSink* mouse_sink_1;
    mi::EventBuilder* mouse_event_builder_1;
    mi::InputSink* mouse_sink_2;
    mi::EventBuilder* mouse_event_builder_2;

    capture_input_sink(device, mouse_sink_1, mouse_event_builder_1);
    capture_input_sink(another_device, mouse_sink_2, mouse_event_builder_2);

    hub.add_device(mt::fake_shared(device));
    hub.add_device(mt::fake_shared(another_device));

    observer_loop.trigger_server_actions();

    auto motion_1 =
        mouse_event_builder_1->pointer_event(arbitrary_timestamp, mir_pointer_action_motion, 0, 0, 0, 23, 20);
    auto motion_2 =
        mouse_event_builder_2->pointer_event(arbitrary_timestamp, mir_pointer_action_motion, 0, 0, 0, 18, -10);
    auto motion_3 =
        mouse_event_builder_2->pointer_event(arbitrary_timestamp, mir_pointer_action_motion, 0, 0, 0, 2, 10);

    EXPECT_CALL(mock_cursor_listener, cursor_moved_to(23, 20));
    EXPECT_CALL(mock_cursor_listener, cursor_moved_to(41, 10));
    EXPECT_CALL(mock_cursor_listener, cursor_moved_to(43, 20));

    mouse_sink_1->handle_input(*motion_1);
    mouse_sink_2->handle_input(*motion_2);
    mouse_sink_1->handle_input(*motion_3);
}

TEST_F(InputDeviceHubTest, tracks_a_single_button_state_from_multiple_pointing_devices)
{
    using namespace ::testing;

    int const x = 0, y = 0;
    MirPointerButtons no_buttons = 0;
    mi::InputSink* mouse_sink_1;
    mi::EventBuilder* mouse_event_builder_1;
    mi::InputSink* mouse_sink_2;
    mi::EventBuilder* mouse_event_builder_2;

    capture_input_sink(device, mouse_sink_1, mouse_event_builder_1);
    capture_input_sink(another_device, mouse_sink_2, mouse_event_builder_2);

    hub.add_device(mt::fake_shared(device));
    hub.add_device(mt::fake_shared(another_device));

    observer_loop.trigger_server_actions();

    auto motion_1 =
        mouse_event_builder_1->pointer_event(arbitrary_timestamp, mir_pointer_action_button_down, mir_pointer_button_primary, 0, 0, 0, 0);
    auto motion_2 =
        mouse_event_builder_2->pointer_event(arbitrary_timestamp, mir_pointer_action_button_down, mir_pointer_button_secondary, 0, 0, 0, 0);
    auto motion_3 =
        mouse_event_builder_1->pointer_event(arbitrary_timestamp, mir_pointer_action_button_up, no_buttons, 0, 0, 0, 0);
    auto motion_4 =
        mouse_event_builder_2->pointer_event(arbitrary_timestamp, mir_pointer_action_button_up, no_buttons, 0, 0, 0, 0);

    InSequence seq;
    EXPECT_CALL(mock_dispatcher, dispatch(mt::ButtonsDown(x, y, mir_pointer_button_primary)));
    EXPECT_CALL(mock_dispatcher, dispatch(mt::ButtonsDown(x, y, mir_pointer_button_primary | mir_pointer_button_secondary)));
    EXPECT_CALL(mock_dispatcher, dispatch(mt::ButtonsUp(x, y, mir_pointer_button_secondary)));
    EXPECT_CALL(mock_dispatcher, dispatch(mt::ButtonsUp(x, y, no_buttons)));

    mouse_sink_1->handle_input(*motion_1);
    mouse_sink_2->handle_input(*motion_2);
    mouse_sink_1->handle_input(*motion_3);
    mouse_sink_1->handle_input(*motion_4);
}
