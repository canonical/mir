/*
 * Copyright © Canonical Ltd.
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
 */

#include "mir/events/event_builders.h"
#include "mir/events/event_private.h" // only needed to validate motion_up/down mapping

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <linux/input.h>

namespace mev = mir::events;
using namespace ::testing;

namespace
{
struct InputEventBuilder : public Test
{
    MirInputDeviceId const device_id = 7;
    std::chrono::nanoseconds const timestamp = std::chrono::nanoseconds(39);
    MirInputEventModifiers const modifiers = mir_input_event_modifier_meta;
};
}

TEST_F(InputEventBuilder, makes_valid_key_event)
{
    MirKeyboardAction const action = mir_keyboard_action_down;
    xkb_keysym_t const keysym = 34;
    int const scan_code = 17;

   auto ev = mev::make_key_event(
       device_id, timestamp,
       action, keysym, scan_code, modifiers);
   auto e = ev.get();

   EXPECT_EQ(mir_event_type_input, mir_event_get_type(e));
   auto ie = mir_event_get_input_event(e);
   EXPECT_EQ(mir_input_event_type_key, mir_input_event_get_type(ie));
   auto kev = mir_input_event_get_keyboard_event(ie);
   EXPECT_EQ(action, mir_keyboard_event_action(kev));
   EXPECT_EQ(keysym, mir_keyboard_event_keysym(kev));
   EXPECT_EQ(scan_code, mir_keyboard_event_scan_code(kev));
   EXPECT_EQ(modifiers, mir_keyboard_event_modifiers(kev));
}

TEST_F(InputEventBuilder, makes_valid_touch_event)
{
    unsigned touch_count = 3;
    MirTouchId touch_ids[] = {7, 9, 4};
    MirTouchAction actions[] = { mir_touch_action_up, mir_touch_action_change, mir_touch_action_change};
    MirTouchTooltype tooltypes[] = {mir_touch_tooltype_unknown, mir_touch_tooltype_finger, mir_touch_tooltype_stylus};
    float x_axis_values[] = { 7, 14.3, 19.6 };
    float y_axis_values[] = { 3, 9, 11 };
    float pressure_values[] = {3, 9, 14.6};
    float touch_major_values[] = {11, 9, 14};
    float touch_minor_values[] = {13, 3, 9.13};
    float size_values[] = {13, 9, 14};

   auto ev = mev::make_touch_event(
       device_id, timestamp,
       modifiers);
   for (unsigned i = 0; i < touch_count; i++)
   {
       mev::add_touch(*ev, touch_ids[i], actions[i], tooltypes[i], x_axis_values[i], y_axis_values[i],
           pressure_values[i], touch_major_values[i], touch_minor_values[i], size_values[i]);
   }
   auto e = ev.get();

   EXPECT_EQ(mir_event_type_input, mir_event_get_type(e));
   auto ie = mir_event_get_input_event(e);
   EXPECT_EQ(mir_input_event_type_touch, mir_input_event_get_type(ie));
   auto tev = mir_input_event_get_touch_event(ie);
   EXPECT_EQ(modifiers, mir_touch_event_modifiers(tev));
   EXPECT_EQ(touch_count, mir_touch_event_point_count(tev));

   for (unsigned i = 0; i < touch_count; i++)
   {
       EXPECT_EQ(touch_ids[i], mir_touch_event_id(tev, i));
       EXPECT_EQ(actions[i], mir_touch_event_action(tev, i));
       EXPECT_EQ(tooltypes[i], mir_touch_event_tooltype(tev, i));
       EXPECT_EQ(x_axis_values[i], mir_touch_event_axis_value(tev, i, mir_touch_axis_x));
       EXPECT_EQ(y_axis_values[i], mir_touch_event_axis_value(tev, i, mir_touch_axis_y));
       EXPECT_EQ(pressure_values[i], mir_touch_event_axis_value(tev, i, mir_touch_axis_pressure));
       EXPECT_EQ(touch_major_values[i], mir_touch_event_axis_value(tev, i, mir_touch_axis_touch_major));
       EXPECT_EQ(touch_minor_values[i], mir_touch_event_axis_value(tev, i, mir_touch_axis_touch_minor));
       EXPECT_EQ(size_values[i], mir_touch_event_axis_value(tev, i, mir_touch_axis_size));
   }   
}

TEST_F(InputEventBuilder, makes_valid_pointer_event)
{
    MirPointerAction action = mir_pointer_action_enter;
    auto depressed_buttons = mir_pointer_button_back | mir_pointer_button_tertiary;
    float x_axis_value = 3.9, y_axis_value = 7.4, hscroll_value = .9, vscroll_value = .3;
    auto const relative_x_value = 0.0;
    auto const relative_y_value = 0.0;
    auto ev = mev::make_pointer_event(
        device_id, timestamp, modifiers,
        action, depressed_buttons, x_axis_value, y_axis_value,
        hscroll_value, vscroll_value, relative_x_value, relative_y_value);
    auto e = ev.get();

    EXPECT_EQ(mir_event_type_input, mir_event_get_type(e));
    auto ie = mir_event_get_input_event(e);
    EXPECT_EQ(mir_input_event_type_pointer, mir_input_event_get_type(ie));
    auto pev = mir_input_event_get_pointer_event(ie);
    EXPECT_EQ(modifiers, mir_pointer_event_modifiers(pev));
    EXPECT_EQ(action, mir_pointer_event_action(pev));
    EXPECT_TRUE(mir_pointer_event_button_state(pev, mir_pointer_button_back));
    EXPECT_TRUE(mir_pointer_event_button_state(pev, mir_pointer_button_tertiary));
    EXPECT_FALSE(mir_pointer_event_button_state(pev, mir_pointer_button_primary));
    EXPECT_FALSE(mir_pointer_event_button_state(pev, mir_pointer_button_secondary));
    EXPECT_FALSE(mir_pointer_event_button_state(pev, mir_pointer_button_forward));
    EXPECT_EQ(x_axis_value, mir_pointer_event_axis_value(pev, mir_pointer_axis_x));
    EXPECT_EQ(y_axis_value, mir_pointer_event_axis_value(pev, mir_pointer_axis_y));
    EXPECT_EQ(hscroll_value, mir_pointer_event_axis_value(pev, mir_pointer_axis_hscroll));
    EXPECT_EQ(vscroll_value, mir_pointer_event_axis_value(pev, mir_pointer_axis_vscroll));
}

// The following three requirements can be removed as soon as we remove android::InputDispatcher, which is the
// only remaining part that relies on the difference between mir_motion_action_pointer_{up,down} and
// mir_motion_action_{up,down} and the difference between mir_motion_action_move and mir_motion_action_hover_move.
TEST_F(InputEventBuilder, maps_single_touch_down_to_motion_down)
{
    MirTouchAction action =  mir_touch_action_down;

    auto ev = mev::make_touch_event(device_id, timestamp, modifiers);
    mev::add_touch(*ev, 0, action, mir_touch_tooltype_finger, 0, 0, 0, 0, 0, 0);
    auto e = ev.get();

    EXPECT_EQ(mir_event_type_input, mir_event_get_type(e));
    auto ie = mir_event_get_input_event(e);
    EXPECT_EQ(mir_input_event_type_touch, mir_input_event_get_type(ie));
    auto tev = mir_input_event_get_touch_event(ie);

    EXPECT_EQ(action, mir_touch_event_action(tev, 0));
}

TEST_F(InputEventBuilder, maps_single_touch_up_to_motion_up)
{
    MirTouchAction action =  mir_touch_action_up;

    auto ev = mev::make_touch_event(device_id, timestamp, modifiers);
    mev::add_touch(*ev, 0, action, mir_touch_tooltype_finger, 0, 0, 0, 0, 0, 0);
    auto e = ev.get();

    EXPECT_EQ(mir_event_type_input, mir_event_get_type(e));
    auto ie = mir_event_get_input_event(e);
    EXPECT_EQ(mir_input_event_type_touch, mir_input_event_get_type(ie));
    auto tev = mir_input_event_get_touch_event(ie);

    EXPECT_EQ(action, mir_touch_event_action(tev, 0));
}

TEST_F(InputEventBuilder, map_to_hover_if_no_button_pressed)
{
    float x_axis_value = 3.9, y_axis_value = 7.4, hscroll_value = .9, vscroll_value = .3;
    auto const relative_x_value = 0.0;
    auto const relative_y_value = 0.0;
    MirPointerAction action = mir_pointer_action_motion;
    auto ev = mev::make_pointer_event(
        device_id, timestamp, modifiers,
        action, 0, x_axis_value, y_axis_value,
        hscroll_value, vscroll_value, relative_x_value, relative_y_value);
    auto e = ev.get();

    auto ie = mir_event_get_input_event(e);
    EXPECT_EQ(mir_input_event_type_pointer, mir_input_event_get_type(ie));
    auto pev = mir_input_event_get_pointer_event(ie);
    EXPECT_EQ(modifiers, mir_pointer_event_modifiers(pev));
    EXPECT_EQ(action, mir_pointer_event_action(pev));
}

TEST_F(InputEventBuilder, when_creating_input_device_state_event_it_has_supplied_properties)
{
    auto const pos_x = 12.1f;
    auto const pos_y = 53.2f;
    auto const button_state = mir_pointer_button_primary|mir_pointer_button_secondary;
    auto const modifiers = mir_input_event_modifier_ctrl_right | mir_input_event_modifier_ctrl;
    std::vector<uint32_t> const pressed_keys = {KEY_LEFTALT, KEY_M};

    auto ev = mev::make_input_configure_event(
        timestamp,
        button_state,
        modifiers,
        pos_x,
        pos_y,
        {
            mev::InputDeviceState{MirInputDeviceId{3}, pressed_keys, 0},
            mev::InputDeviceState{MirInputDeviceId{2}, {}, button_state}});

    EXPECT_THAT(mir_event_get_type(ev.get()), Eq(mir_event_type_input_device_state));
    auto ids_event = mir_event_get_input_device_state_event(ev.get());

    EXPECT_THAT(mir_input_device_state_event_time(ids_event), Eq(timestamp.count()));
    EXPECT_THAT(mir_input_device_state_event_modifiers(ids_event), Eq(modifiers));
    EXPECT_THAT(mir_input_device_state_event_pointer_axis(ids_event, mir_pointer_axis_x), Eq(pos_x));
    EXPECT_THAT(mir_input_device_state_event_pointer_axis(ids_event, mir_pointer_axis_y), Eq(pos_y));
    EXPECT_THAT(mir_input_device_state_event_pointer_buttons(ids_event), Eq(button_state));
    EXPECT_THAT(mir_input_device_state_event_device_count(ids_event), Eq(2));

    EXPECT_THAT(mir_input_device_state_event_device_id(ids_event, 0), Eq(MirInputDeviceId{3}));
    auto const pressed_keys_count = mir_input_device_state_event_device_pressed_keys_count(ids_event, 0);
    EXPECT_THAT(pressed_keys_count, Eq(2));
    for (uint32_t i = 0; i < pressed_keys_count; i++)
    {
        EXPECT_THAT(mir_input_device_state_event_device_pressed_keys_for_index(ids_event, 0, i), Eq(pressed_keys[i]));
    }
    EXPECT_THAT(mir_input_device_state_event_device_pointer_buttons(ids_event, 0), Eq(0));

    EXPECT_THAT(mir_input_device_state_event_device_id(ids_event, 1), Eq(MirInputDeviceId{2}));
    EXPECT_THAT(mir_input_device_state_event_device_pressed_keys_count(ids_event, 1), Eq(0));
    EXPECT_THAT(mir_input_device_state_event_device_pointer_buttons(ids_event, 1), Eq(button_state));
}
