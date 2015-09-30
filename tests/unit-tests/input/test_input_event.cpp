/*
 * Copyright © 2014 Canonical Ltd.
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

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "mir/events/event_private.h"
#include "mir_toolkit/events/input/input_event.h"

using namespace testing;

// See: https://bugs.launchpad.net/mir/+bug/1311699
#define MIR_EVENT_ACTION_POINTER_INDEX_MASK 0xff00
#define MIR_EVENT_ACTION_POINTER_INDEX_SHIFT 8;
#define MIR_EVENT_ACTION_MASK 0xff

// TODO: Refactor event initializers

namespace
{
// Never exposed in old event, so lets avoid leaking it in to a header now.
enum 
{
    AINPUT_SOURCE_CLASS_MASK = 0x000000ff,

    AINPUT_SOURCE_CLASS_BUTTON = 0x00000001,
    AINPUT_SOURCE_CLASS_POINTER = 0x00000002,
    AINPUT_SOURCE_CLASS_NAVIGATION = 0x00000004,
    AINPUT_SOURCE_CLASS_POSITION = 0x00000008,
    AINPUT_SOURCE_CLASS_JOYSTICK = 0x00000010
};
enum 
{
    AINPUT_SOURCE_UNKNOWN = 0x00000000,

    AINPUT_SOURCE_KEYBOARD = 0x00000100 | AINPUT_SOURCE_CLASS_BUTTON,
    AINPUT_SOURCE_DPAD = 0x00000200 | AINPUT_SOURCE_CLASS_BUTTON,
    AINPUT_SOURCE_GAMEPAD = 0x00000400 | AINPUT_SOURCE_CLASS_BUTTON,
    AINPUT_SOURCE_TOUCHSCREEN = 0x00001000 | AINPUT_SOURCE_CLASS_POINTER,
    AINPUT_SOURCE_MOUSE = 0x00002000 | AINPUT_SOURCE_CLASS_POINTER,
    AINPUT_SOURCE_STYLUS = 0x00004000 | AINPUT_SOURCE_CLASS_POINTER,
    AINPUT_SOURCE_TRACKBALL = 0x00010000 | AINPUT_SOURCE_CLASS_NAVIGATION,
    AINPUT_SOURCE_TOUCHPAD = 0x00100000 | AINPUT_SOURCE_CLASS_POSITION,
    AINPUT_SOURCE_JOYSTICK = 0x01000000 | AINPUT_SOURCE_CLASS_JOYSTICK,

    AINPUT_SOURCE_ANY = 0xffffff00
};

MirEvent a_key_ev()
{
    MirEvent key_ev;
    memset(&key_ev, 0, sizeof(key_ev));
    
    key_ev.type = mir_event_type_key;
    
    return key_ev;
}

MirEvent a_motion_ev(int device_class = AINPUT_SOURCE_UNKNOWN)
{
    MirEvent motion_ev;
    memset(&motion_ev, 0, sizeof(motion_ev));
    
    motion_ev.type = mir_event_type_motion;
    motion_ev.motion.source_id = device_class;
    
    return motion_ev;
}

}

TEST(InputEvent, old_style_key_and_motion_events_are_input_events)
{
    auto key_ev = a_key_ev();
    
    EXPECT_EQ(mir_event_type_input, mir_event_get_type(&key_ev));
    EXPECT_EQ(mir_input_event_type_key, mir_input_event_get_type(mir_event_get_input_event(&key_ev)));

    auto motion_ev = a_motion_ev();
    
    EXPECT_EQ(mir_event_type_input, mir_event_get_type(&motion_ev));
    EXPECT_NE(nullptr, mir_event_get_input_event(&motion_ev));
}

// MirInputEvent properties common to all events.

TEST(CommonInputEventProperties, device_id_taken_from_old_style_event)
{
    int64_t dev_id_1 = 17, dev_id_2 = 23;
    auto old_ev = a_motion_ev();

    old_ev.type = mir_event_type_motion;
    old_ev.motion.device_id = dev_id_1;
    EXPECT_EQ(dev_id_1, mir_input_event_get_device_id(
        mir_event_get_input_event(&old_ev)));

    old_ev.type = mir_event_type_key;
    old_ev.motion.device_id = dev_id_2;
    EXPECT_EQ(dev_id_2, mir_input_event_get_device_id(
        mir_event_get_input_event(&old_ev)));
}

TEST(CommonInputEventProperties, event_time_taken_from_old_style_event)
{
    std::chrono::nanoseconds event_time_1{79}, event_time_2{83};
    auto old_ev = a_motion_ev();

    old_ev.motion.event_time = event_time_1;
    EXPECT_EQ(event_time_1.count(), mir_input_event_get_event_time(
        mir_event_get_input_event(&old_ev)));

    old_ev.type = mir_event_type_key;
    old_ev.key.event_time = event_time_2;
    EXPECT_EQ(event_time_2.count(), mir_input_event_get_event_time(
        mir_event_get_input_event(&old_ev)));
}

TEST(KeyInputEventProperties, timestamp_taken_from_old_style_event)
{
    std::chrono::nanoseconds event_time_1{79}, event_time_2{83};
    auto old_ev = a_key_ev();
    auto const keyboard_event = mir_input_event_get_keyboard_event(mir_event_get_input_event(&old_ev));

    for (auto expected : {event_time_1, event_time_2})
    {
        old_ev.key.event_time = expected;

        auto const input_event = mir_keyboard_event_input_event(keyboard_event);

        EXPECT_THAT(mir_input_event_get_event_time(input_event), Eq(expected.count()));
    }
}


TEST(KeyInputEventProperties, up_and_down_actions_copied_from_old_style_event)
{
    auto old_ev = a_key_ev();

    old_ev.key.action = mir_keyboard_action_down;
    
    auto new_kev = mir_input_event_get_keyboard_event(mir_event_get_input_event(&old_ev));
    EXPECT_EQ(mir_keyboard_action_down, mir_keyboard_event_action(new_kev));

    old_ev.key.action = mir_keyboard_action_up;
    EXPECT_EQ(mir_keyboard_action_up, mir_keyboard_event_action(new_kev));
}

TEST(KeyInputEventProperties, keycode_scancode_and_modifiers_taken_from_old_style_event)
{
    xkb_keysym_t key_code = 171;
    int scan_code = 31;
    MirInputEventModifiers modifiers = mir_input_event_modifier_shift;

    auto old_ev = a_key_ev();
    old_ev.key.key_code = key_code;
    old_ev.key.scan_code = scan_code;
    old_ev.key.modifiers = modifiers;

    auto new_kev = mir_input_event_get_keyboard_event(mir_event_get_input_event(&old_ev));
    EXPECT_EQ(key_code, mir_keyboard_event_key_code(new_kev));
    EXPECT_EQ(scan_code, mir_keyboard_event_scan_code(new_kev));
    EXPECT_EQ(modifiers, mir_keyboard_event_modifiers(new_kev));
}

TEST(TouchEventProperties, timestamp_taken_from_old_style_event)
{
    std::chrono::nanoseconds event_time_1{79}, event_time_2{83};
    auto old_ev = a_motion_ev(AINPUT_SOURCE_TOUCHSCREEN);
    auto const touch_event = mir_input_event_get_touch_event(mir_event_get_input_event(&old_ev));

    for (auto expected : {event_time_1, event_time_2})
    {
        old_ev.motion.event_time = expected;

        auto const input_event = mir_touch_event_input_event(touch_event);

        EXPECT_THAT(mir_input_event_get_event_time(input_event), Eq(expected.count()));
    }
}

TEST(TouchEventProperties, touch_count_taken_from_pointer_count)
{
    unsigned const pointer_count = 3;
    auto old_ev = a_motion_ev(AINPUT_SOURCE_TOUCHSCREEN);

    old_ev.motion.pointer_count = pointer_count;
    
    auto tev = mir_input_event_get_touch_event(mir_event_get_input_event(&old_ev));
    EXPECT_EQ(pointer_count, mir_touch_event_point_count(tev));
}

TEST(TouchEventProperties, touch_id_comes_from_pointer_coordinates)
{
    unsigned const touch_id = 31;
    auto old_ev = a_motion_ev(AINPUT_SOURCE_TOUCHSCREEN);

    old_ev.motion.pointer_count = 1;
    old_ev.motion.pointer_coordinates[0].id = touch_id;

    auto tev = mir_input_event_get_touch_event(mir_event_get_input_event(&old_ev));
    EXPECT_EQ(touch_id, mir_touch_event_id(tev, 0));
}

// mir_motion_action_up/down represent the start of a gesture. pointers only go up/down one at a time
TEST(TouchEventProperties, down_and_up_actions_are_taken_from_old_event)
{
    auto old_ev = a_motion_ev(AINPUT_SOURCE_TOUCHSCREEN);
    old_ev.motion.pointer_count = 1;
    old_ev.motion.pointer_coordinates[0].action = mir_touch_action_change;

    auto tev = mir_input_event_get_touch_event(mir_event_get_input_event(&old_ev));
    EXPECT_EQ(mir_touch_action_change, mir_touch_event_action(tev, 0));
}

TEST(TouchEventProperties, tool_type_copied_from_old_pc)
{
    auto old_ev = a_motion_ev(AINPUT_SOURCE_TOUCHSCREEN);

    auto& old_mev = old_ev.motion;
    old_mev.pointer_count = 3;
    old_mev.pointer_coordinates[0].tool_type = mir_touch_tooltype_unknown;
    old_mev.pointer_coordinates[1].tool_type = mir_touch_tooltype_finger;
    old_mev.pointer_coordinates[2].tool_type = mir_touch_tooltype_stylus;

    auto tev = mir_input_event_get_touch_event(mir_event_get_input_event(&old_ev));
    EXPECT_EQ(mir_touch_tooltype_unknown, mir_touch_event_tooltype(tev, 0));
    EXPECT_EQ(mir_touch_tooltype_finger, mir_touch_event_tooltype(tev, 1));
    EXPECT_EQ(mir_touch_tooltype_stylus, mir_touch_event_tooltype(tev, 2));
}

TEST(TouchEventProperties, axis_values_used_by_qtmir_copied)
{
    float x_value = 19, y_value = 23, touch_major = .3, touch_minor = .2, pressure = .9, size = 1111;
    auto old_ev = a_motion_ev(AINPUT_SOURCE_TOUCHSCREEN);
    old_ev.motion.pointer_count = 1;
    auto &old_pc = old_ev.motion.pointer_coordinates[0];
    old_pc.x = x_value;
    old_pc.y = y_value;
    old_pc.touch_major = touch_major;
    old_pc.touch_minor = touch_minor;
    old_pc.pressure = pressure;
    old_pc.size = size;

    auto tev = mir_input_event_get_touch_event(mir_event_get_input_event(&old_ev));
    EXPECT_EQ(x_value, mir_touch_event_axis_value(tev, 0, mir_touch_axis_x));
    EXPECT_EQ(y_value, mir_touch_event_axis_value(tev, 0, mir_touch_axis_y));
    EXPECT_EQ(touch_major, mir_touch_event_axis_value(tev, 0, mir_touch_axis_touch_major));
    EXPECT_EQ(touch_minor, mir_touch_event_axis_value(tev, 0, mir_touch_axis_touch_minor));
    EXPECT_EQ(pressure, mir_touch_event_axis_value(tev, 0, mir_touch_axis_pressure));
    EXPECT_EQ(size, mir_touch_event_axis_value(tev, 0, mir_touch_axis_size));
}

/* Pointer and touch event differentiation */

namespace
{
struct DeviceClassTestParameters
{
    MirInputEventType expected_type;
    int32_t device_class;
};

struct DeviceClassTest : public testing::Test, testing::WithParamInterface<DeviceClassTestParameters>
{
};
}

TEST_P(DeviceClassTest, pointer_and_touch_events_differentiated_on_device_class)
{
    auto const& param = GetParam();
    auto old_ev = a_motion_ev(param.device_class);

    auto iev = mir_event_get_input_event(&old_ev);
    EXPECT_EQ(param.expected_type, mir_input_event_get_type(iev));
}

INSTANTIATE_TEST_CASE_P(MouseDeviceClassTest,
    DeviceClassTest, ::testing::Values(DeviceClassTestParameters{mir_input_event_type_pointer, AINPUT_SOURCE_MOUSE}));

INSTANTIATE_TEST_CASE_P(TouchpadDeviceClassTest,
    DeviceClassTest, ::testing::Values(DeviceClassTestParameters{mir_input_event_type_pointer, AINPUT_SOURCE_TOUCHPAD}));

INSTANTIATE_TEST_CASE_P(TrackballDeviceClassTest,
    DeviceClassTest, ::testing::Values(DeviceClassTestParameters{mir_input_event_type_pointer, AINPUT_SOURCE_TRACKBALL}));

INSTANTIATE_TEST_CASE_P(TouchscreenDeviceClassTest,
    DeviceClassTest, ::testing::Values(DeviceClassTestParameters{mir_input_event_type_touch, AINPUT_SOURCE_TOUCHSCREEN}));

INSTANTIATE_TEST_CASE_P(StylusDeviceClassTest,
    DeviceClassTest, ::testing::Values(DeviceClassTestParameters{mir_input_event_type_touch, AINPUT_SOURCE_STYLUS}));

/* Pointer event property accessors */

TEST(PointerInputEventProperties, timestamp_taken_from_old_style_event)
{
    std::chrono::nanoseconds event_time_1{79}, event_time_2{83};
    auto old_ev = a_motion_ev(AINPUT_SOURCE_MOUSE);
    auto const pointer_event = mir_input_event_get_pointer_event(mir_event_get_input_event(&old_ev));

    for (auto expected : {event_time_1, event_time_2})
    {
        old_ev.motion.event_time = expected;

        auto const input_event = mir_pointer_event_input_event(pointer_event);

        EXPECT_THAT(mir_input_event_get_event_time(input_event), Eq(expected.count()));
    }
}

TEST(PointerInputEventProperties, modifiers_taken_from_old_style_ev)
{
    MirInputEventModifiers modifiers = mir_input_event_modifier_shift;
    auto old_ev = a_motion_ev(AINPUT_SOURCE_MOUSE);
    old_ev.motion.modifiers = modifiers;
    
    auto pointer_event = 
        mir_input_event_get_pointer_event(mir_event_get_input_event(&old_ev));
    EXPECT_EQ(modifiers, mir_pointer_event_modifiers(pointer_event));
}

TEST(PointerInputEventProperties, button_state_translated)
{
    auto old_ev = a_motion_ev(AINPUT_SOURCE_MOUSE);

    old_ev.motion.buttons = mir_pointer_button_primary;
    auto pev = mir_input_event_get_pointer_event(mir_event_get_input_event(&old_ev));
    
    EXPECT_TRUE(mir_pointer_event_button_state(pev, mir_pointer_button_primary));
    EXPECT_FALSE(mir_pointer_event_button_state(pev, mir_pointer_button_secondary));

    old_ev.motion.buttons |=  mir_pointer_button_secondary;

    EXPECT_TRUE(mir_pointer_event_button_state(pev, mir_pointer_button_primary));
    EXPECT_TRUE(mir_pointer_event_button_state(pev, mir_pointer_button_secondary));

    EXPECT_FALSE(mir_pointer_event_button_state(pev, mir_pointer_button_tertiary));
    EXPECT_FALSE(mir_pointer_event_button_state(pev, mir_pointer_button_back));
    EXPECT_FALSE(mir_pointer_event_button_state(pev, mir_pointer_button_forward));
}

TEST(PointerInputEventProperties, axis_values_copied)
{
    float x = 7, y = 9.3, hscroll = 13, vscroll = 17;
    auto old_ev = a_motion_ev(AINPUT_SOURCE_MOUSE);
    old_ev.motion.pointer_count = 0;
    old_ev.motion.pointer_coordinates[0].x = x;
    old_ev.motion.pointer_coordinates[0].y = y;
    old_ev.motion.pointer_coordinates[0].vscroll = vscroll;
    old_ev.motion.pointer_coordinates[0].hscroll = hscroll;

    auto pev = mir_input_event_get_pointer_event(mir_event_get_input_event(&old_ev));
    EXPECT_EQ(x, mir_pointer_event_axis_value(pev, mir_pointer_axis_x));
    EXPECT_EQ(y, mir_pointer_event_axis_value(pev, mir_pointer_axis_y));
    EXPECT_EQ(vscroll, mir_pointer_event_axis_value(pev, mir_pointer_axis_vscroll));
    EXPECT_EQ(hscroll, mir_pointer_event_axis_value(pev, mir_pointer_axis_hscroll));
}
