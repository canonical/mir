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

#include "src/server/input/seat_input_device_tracker.h"
#include "src/server/input/default_event_builder.h"

#include "mir/test/doubles/mock_input_device.h"
#include "mir/test/doubles/mock_input_dispatcher.h"
#include "mir/test/doubles/mock_input_region.h"
#include "mir/test/doubles/mock_cursor_listener.h"
#include "mir/test/doubles/mock_touch_visualizer.h"
#include "mir/test/event_matchers.h"
#include "mir/test/fake_shared.h"

#include "mir/cookie/authority.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <linux/input.h>

namespace mt = mir::test;
namespace mtd = mt::doubles;
namespace mi = mir::input;
namespace
{

template<typename Type>
using Nice = ::testing::NiceMock<Type>;
using namespace ::testing;

struct SeatInputDeviceTracker : ::testing::Test
{
    mi::EventBuilder* builder;
    Nice<mtd::MockInputDispatcher> mock_dispatcher;
    Nice<mtd::MockInputRegion> mock_region;
    Nice<mtd::MockCursorListener> mock_cursor_listener;
    Nice<mtd::MockTouchVisualizer> mock_visualizer;
    MirInputDeviceId some_device{8712};
    MirInputDeviceId another_device{1246};
    MirInputDeviceId third_device{86};
    std::shared_ptr<mir::cookie::Authority> cookie_factory = mir::cookie::Authority::create();

    mi::DefaultEventBuilder some_device_builder{some_device, cookie_factory};
    mi::DefaultEventBuilder another_device_builder{another_device, cookie_factory};
    mi::DefaultEventBuilder third_device_builder{third_device, cookie_factory};
    mi::SeatInputDeviceTracker tracker{mt::fake_shared(mock_dispatcher), mt::fake_shared(mock_visualizer),
                       mt::fake_shared(mock_cursor_listener), mt::fake_shared(mock_region)};

    std::chrono::nanoseconds arbitrary_timestamp;
};

}

TEST_F(SeatInputDeviceTracker, throws_on_unknown_device)
{
    EXPECT_CALL(mock_dispatcher, dispatch(_)).Times(0);
    EXPECT_THROW(
        {
            tracker.dispatch(*some_device_builder.touch_event(arbitrary_timestamp));
        },
        std::logic_error);
}

TEST_F(SeatInputDeviceTracker, dispatch_posts_to_input_dispatcher)
{
    EXPECT_CALL(mock_dispatcher, dispatch(_));

    tracker.add_device(some_device);
    tracker.dispatch(*some_device_builder.touch_event(arbitrary_timestamp));
}

TEST_F(SeatInputDeviceTracker, forwards_touch_spots_to_visualizer)
{
    using Spot = mi::TouchVisualizer::Spot;
    InSequence seq;
    EXPECT_CALL(mock_visualizer, visualize_touches(ElementsAreArray({Spot{{4, 2}, 10}})));
    EXPECT_CALL(mock_visualizer, visualize_touches(ElementsAreArray({Spot{{10, 10}, 30}, Spot{{100, 34}, 0}})));
    EXPECT_CALL(mock_visualizer, visualize_touches(ElementsAreArray({Spot{{70, 10}, 30}})));
    EXPECT_CALL(mock_visualizer, visualize_touches(ElementsAreArray(std::vector<Spot>())));

    tracker.add_device(some_device);

    auto touch_event_1 = some_device_builder.touch_event(arbitrary_timestamp);
    some_device_builder.add_touch(*touch_event_1, 0, mir_touch_action_down, mir_touch_tooltype_finger, 4.0f, 2.0f,
                                  10.0f, 15.0f, 5.0f, 4.0f);

    auto touch_event_2 = some_device_builder.touch_event(arbitrary_timestamp);
    some_device_builder.add_touch(*touch_event_2, 0, mir_touch_action_change, mir_touch_tooltype_finger, 10.0f, 10.0f,
                                  30.0f, 15.0f, 5.0f, 4.0f);
    some_device_builder.add_touch(*touch_event_2, 1, mir_touch_action_down, mir_touch_tooltype_finger, 100.0f, 34.0f,
                                  0.0f, 15.0f, 5.0f, 4.0f);

    auto touch_event_3 = some_device_builder.touch_event(arbitrary_timestamp);
    some_device_builder.add_touch(*touch_event_3, 0, mir_touch_action_up, mir_touch_tooltype_finger, 100.0f, 34.0f,
                                  0.0f, 15.0f, 5.0f, 4.0f);
    some_device_builder.add_touch(*touch_event_3, 1, mir_touch_action_change, mir_touch_tooltype_finger, 70.0f, 10.0f,
                                  30.0f, 15.0f, 5.0f, 4.0f);

    auto touch_event_4 = some_device_builder.touch_event(arbitrary_timestamp);
    some_device_builder.add_touch(*touch_event_4, 1, mir_touch_action_up, mir_touch_tooltype_finger, 70.0f, 10.0f,
                                  30.0f, 15.0f, 5.0f, 4.0f);

    tracker.dispatch(*touch_event_1);
    tracker.dispatch(*touch_event_2);
    tracker.dispatch(*touch_event_3);
    tracker.dispatch(*touch_event_4);
}

TEST_F(SeatInputDeviceTracker, removal_of_touch_device_removes_spots)
{
    using Spot = mi::TouchVisualizer::Spot;
    InSequence seq;
    EXPECT_CALL(mock_visualizer, visualize_touches(ElementsAreArray({Spot{{4, 2}, 10}})));
    EXPECT_CALL(mock_visualizer, visualize_touches(ElementsAreArray(std::vector<Spot>())));

    tracker.add_device(some_device);

    auto touch_event_1 = some_device_builder.touch_event(arbitrary_timestamp);
    some_device_builder.add_touch(*touch_event_1, 0, mir_touch_action_down, mir_touch_tooltype_finger, 4.0f, 2.0f,
                                  10.0f, 15.0f, 5.0f, 4.0f);

    tracker.dispatch(*touch_event_1);
    tracker.remove_device(some_device);
}

TEST_F(SeatInputDeviceTracker, pointer_movement_updates_cursor)
{
    auto const move_x = 20.0f, move_y = 40.0f;
    EXPECT_CALL(mock_cursor_listener, cursor_moved_to(move_x, move_y)).Times(1);

    tracker.add_device(some_device);
    tracker.dispatch(*some_device_builder.pointer_event(arbitrary_timestamp, mir_pointer_action_motion, 0, 0.0f, 0.0f,
                                                        move_x, move_y));
}

TEST_F(SeatInputDeviceTracker, key_strokes_of_modifier_key_update_modifier)
{
    const MirInputEventModifiers shift_left = mir_input_event_modifier_shift_left | mir_input_event_modifier_shift;
    InSequence seq;
    EXPECT_CALL(mock_dispatcher, dispatch(mt::KeyWithModifiers(shift_left)));
    EXPECT_CALL(mock_dispatcher, dispatch(mt::KeyWithModifiers(mir_input_event_modifier_none)));

    tracker.add_device(some_device);
    tracker.dispatch(*some_device_builder.key_event(arbitrary_timestamp, mir_keyboard_action_down, 0, KEY_LEFTSHIFT));
    tracker.dispatch(*some_device_builder.key_event(arbitrary_timestamp, mir_keyboard_action_up, 0, KEY_LEFTSHIFT));
}

TEST_F(SeatInputDeviceTracker, modifier_events_on_different_keyboards_do_not_change_modifier_state)
{
    const MirInputEventModifiers shift_left = mir_input_event_modifier_shift_left | mir_input_event_modifier_shift;
    InSequence seq;
    EXPECT_CALL(mock_dispatcher, dispatch(mt::KeyWithModifiers(shift_left)));
    EXPECT_CALL(mock_dispatcher, dispatch(mt::KeyWithModifiers(mir_input_event_modifier_none)));

    tracker.add_device(some_device);
    tracker.add_device(another_device);
    tracker.dispatch(*some_device_builder.key_event(arbitrary_timestamp, mir_keyboard_action_down, 0, KEY_LEFTSHIFT));
    tracker.dispatch(*another_device_builder.key_event(arbitrary_timestamp, mir_keyboard_action_up, 0, KEY_A));
}

TEST_F(SeatInputDeviceTracker, modifier_events_on_different_keyboards_contribute_to_pointer_event_modifier_state)
{
    const MirInputEventModifiers r_alt_modifier = mir_input_event_modifier_alt_right | mir_input_event_modifier_alt;
    const MirInputEventModifiers shift_right = mir_input_event_modifier_shift_right | mir_input_event_modifier_shift;
    InSequence seq;
    EXPECT_CALL(mock_dispatcher, dispatch(mt::KeyWithModifiers(r_alt_modifier)));
    EXPECT_CALL(mock_dispatcher, dispatch(mt::KeyWithModifiers(shift_right)));
    EXPECT_CALL(mock_dispatcher, dispatch(mt::PointerEventWithModifiers(shift_right | r_alt_modifier)));

    tracker.add_device(some_device);
    tracker.add_device(another_device);
    tracker.add_device(third_device);
    tracker.dispatch(*some_device_builder.key_event(arbitrary_timestamp, mir_keyboard_action_down, 0, KEY_RIGHTALT));
    tracker.dispatch(
        *another_device_builder.key_event(arbitrary_timestamp, mir_keyboard_action_down, 0, KEY_RIGHTSHIFT));
    tracker.dispatch(
        *third_device_builder.pointer_event(arbitrary_timestamp, mir_pointer_action_motion, 0, 0, 0, 12, 40));
}

TEST_F(SeatInputDeviceTracker, device_removal_removes_modifier_flags)
{
    const MirInputEventModifiers r_alt_modifier = mir_input_event_modifier_alt_right | mir_input_event_modifier_alt;
    const MirInputEventModifiers shift_right = mir_input_event_modifier_shift_right | mir_input_event_modifier_shift;
    InSequence seq;
    EXPECT_CALL(mock_dispatcher, dispatch(mt::KeyWithModifiers(r_alt_modifier)));
    EXPECT_CALL(mock_dispatcher, dispatch(mt::KeyWithModifiers(shift_right)));
    EXPECT_CALL(mock_dispatcher, dispatch(mt::PointerEventWithModifiers(r_alt_modifier)));

    tracker.add_device(some_device);
    tracker.add_device(another_device);
    tracker.add_device(third_device);
    tracker.dispatch(*some_device_builder.key_event(arbitrary_timestamp, mir_keyboard_action_down, 0, KEY_RIGHTALT));
    tracker.dispatch(
        *another_device_builder.key_event(arbitrary_timestamp, mir_keyboard_action_down, 0, KEY_RIGHTSHIFT));
    tracker.remove_device(another_device);
    tracker.dispatch(
        *third_device_builder.pointer_event(arbitrary_timestamp, mir_pointer_action_motion, 0, 0, 0, 12, 40));
}

TEST_F(SeatInputDeviceTracker, pointer_movement_from_different_devices_change_cursor_position)
{
    InSequence seq;
    EXPECT_CALL(mock_cursor_listener, cursor_moved_to(23, 20));
    EXPECT_CALL(mock_cursor_listener, cursor_moved_to(41, 10));
    EXPECT_CALL(mock_cursor_listener, cursor_moved_to(43, 20));

    tracker.add_device(some_device);
    tracker.add_device(another_device);

    tracker.dispatch(
        *some_device_builder.pointer_event(arbitrary_timestamp, mir_pointer_action_motion, 0, 0, 0, 23, 20));
    tracker.dispatch(
        *another_device_builder.pointer_event(arbitrary_timestamp, mir_pointer_action_motion, 0, 0, 0, 18, -10));
    tracker.dispatch(
        *another_device_builder.pointer_event(arbitrary_timestamp, mir_pointer_action_motion, 0, 0, 0, 2, 10));
}

TEST_F(SeatInputDeviceTracker, tracks_a_single_button_state_for_multiple_pointing_devices)
{
    int const x = 0, y = 0;
    MirPointerButtons no_buttons = 0;
    InSequence seq;
    EXPECT_CALL(mock_dispatcher, dispatch(mt::ButtonsDown(x, y, mir_pointer_button_primary)));
    EXPECT_CALL(mock_dispatcher,
                dispatch(mt::ButtonsDown(x, y, mir_pointer_button_primary | mir_pointer_button_secondary)));
    EXPECT_CALL(mock_dispatcher, dispatch(mt::ButtonsUp(x, y, mir_pointer_button_secondary)));
    EXPECT_CALL(mock_dispatcher, dispatch(mt::ButtonsUp(x, y, no_buttons)));

    tracker.add_device(some_device);
    tracker.add_device(another_device);

    tracker.dispatch(*some_device_builder.pointer_event(arbitrary_timestamp, mir_pointer_action_button_down,
                                                        mir_pointer_button_primary, 0, 0, 0, 0));
    tracker.dispatch(*another_device_builder.pointer_event(arbitrary_timestamp, mir_pointer_action_button_down,
                                                           mir_pointer_button_secondary, 0, 0, 0, 0));
    tracker.dispatch(
        *some_device_builder.pointer_event(arbitrary_timestamp, mir_pointer_action_button_up, no_buttons, 0, 0, 0, 0));
    tracker.dispatch(*another_device_builder.pointer_event(arbitrary_timestamp, mir_pointer_action_button_up,
                                                           no_buttons, 0, 0, 0, 0));
}

TEST_F(SeatInputDeviceTracker, poiniting_device_removal_removes_pressed_button_state)
{
    int const x = 0, y = 0;

    InSequence seq;
    EXPECT_CALL(mock_dispatcher, dispatch(mt::ButtonsDown(x, y, mir_pointer_button_primary)));
    EXPECT_CALL(mock_dispatcher,
                dispatch(mt::ButtonsDown(x, y, mir_pointer_button_primary | mir_pointer_button_secondary)));
    EXPECT_CALL(mock_dispatcher, dispatch(mt::ButtonsDown(x, y, mir_pointer_button_secondary)));

    tracker.add_device(some_device);
    tracker.add_device(another_device);
    tracker.dispatch(*some_device_builder.pointer_event(arbitrary_timestamp, mir_pointer_action_button_down,
                                                        mir_pointer_button_primary, 0, 0, 0, 0));
    tracker.dispatch(*another_device_builder.pointer_event(arbitrary_timestamp, mir_pointer_action_button_down,
                                                           mir_pointer_button_secondary, 0, 0, 0, 0));
    tracker.remove_device(some_device);
    tracker.dispatch(*another_device_builder.pointer_event(arbitrary_timestamp, mir_pointer_action_motion,
                                                           mir_pointer_button_secondary, 0, 0, 0, 0));
}
