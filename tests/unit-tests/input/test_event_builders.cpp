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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#define MIR_INCLUDE_DEPRECATED_EVENT_HEADER

#include "mir/events/event_builders.h"

#include <gtest/gtest.h>

namespace mev = mir::events;

namespace
{
struct InputEventBuilder : public testing::Test
{
    MirInputDeviceId const device_id = 7;
    int64_t const timestamp = 39;
    MirInputEventModifiers const modifiers = mir_input_event_modifier_meta;
};
}

TEST_F(InputEventBuilder, makes_valid_key_event)
{
    MirKeyInputEventAction const action = mir_key_input_event_action_down;
    xkb_keysym_t const key_code = 34;
    int const scan_code = 17;

   auto ev = mev::make_event(device_id, timestamp,
       action, key_code, scan_code, modifiers);
   auto e = ev.get();

   EXPECT_EQ(mir_event_type_input, mir_event_get_type(e));
   auto ie = mir_event_get_input_event(e);
   EXPECT_EQ(mir_input_event_type_key, mir_input_event_get_type(ie));
   auto kev = mir_input_event_get_key_input_event(ie);
   EXPECT_EQ(action, mir_key_input_event_get_action(kev));
   EXPECT_EQ(key_code, mir_key_input_event_get_key_code(kev));
   EXPECT_EQ(scan_code, mir_key_input_event_get_scan_code(kev));
   EXPECT_EQ(modifiers, mir_key_input_event_get_modifiers(kev));
}


TEST_F(InputEventBuilder, makes_valid_touch_event)
{
    unsigned touch_count = 3;
    MirTouchInputEventTouchId touch_ids[] = {7, 9, 4};
    MirTouchInputEventTouchAction actions[] = { mir_touch_input_event_action_up, mir_touch_input_event_action_change, mir_touch_input_event_action_change};
    MirTouchInputEventTouchTooltype tooltypes[] = {mir_touch_input_tool_type_unknown, mir_touch_input_tool_type_finger, mir_touch_input_tool_type_stylus};
    float x_axis_values[] = { 7, 14.3, 19.6 };
    float y_axis_values[] = { 3, 9, 11 };
    float pressure_values[] = {3, 9, 14.6};
    float touch_major_values[] = {11, 9, 14};
    float touch_minor_values[] = {13, 3, 9.13};
    float size_values[] = {4, 9, 6};

   auto ev = mev::make_event(device_id, timestamp,
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
   auto tev = mir_input_event_get_touch_input_event(ie);
   EXPECT_EQ(modifiers, mir_touch_input_event_get_modifiers(tev));
   EXPECT_EQ(touch_count, mir_touch_input_event_get_touch_count(tev));

   for (unsigned i = 0; i < touch_count; i++)
   {
       EXPECT_EQ(touch_ids[i], mir_touch_input_event_get_touch_id(tev, i));
       EXPECT_EQ(actions[i], mir_touch_input_event_get_touch_action(tev, i));
       EXPECT_EQ(tooltypes[i], mir_touch_input_event_get_touch_tooltype(tev, i));
       EXPECT_EQ(x_axis_values[i], mir_touch_input_event_get_touch_axis_value(tev, i, mir_touch_input_axis_x));
       EXPECT_EQ(y_axis_values[i], mir_touch_input_event_get_touch_axis_value(tev, i, mir_touch_input_axis_y));
       EXPECT_EQ(pressure_values[i], mir_touch_input_event_get_touch_axis_value(tev, i, mir_touch_input_axis_pressure));
       EXPECT_EQ(touch_major_values[i], mir_touch_input_event_get_touch_axis_value(tev, i, mir_touch_input_axis_touch_major));
       EXPECT_EQ(touch_minor_values[i], mir_touch_input_event_get_touch_axis_value(tev, i, mir_touch_input_axis_touch_minor));
       EXPECT_EQ(size_values[i], mir_touch_input_event_get_touch_axis_value(tev, i, mir_touch_input_axis_size));
   }   
}

TEST_F(InputEventBuilder, makes_valid_pointer_event)
{
    MirPointerInputEventAction action = mir_pointer_input_event_action_enter;
    std::vector<MirPointerInputEventButton> depressed_buttons = 
        {mir_pointer_input_button_back, mir_pointer_input_button_tertiary};
    float x_axis_value = 3.9, y_axis_value = 7.4, hscroll_value = .9, vscroll_value = .3;
    auto ev = mev::make_event(device_id, timestamp, modifiers, 
        action, depressed_buttons, x_axis_value, y_axis_value, hscroll_value, vscroll_value);
    auto e = ev.get();

    EXPECT_EQ(mir_event_type_input, mir_event_get_type(e));
    auto ie = mir_event_get_input_event(e);
    EXPECT_EQ(mir_input_event_type_pointer, mir_input_event_get_type(ie));
    auto pev = mir_input_event_get_pointer_input_event(ie);
    EXPECT_EQ(modifiers, mir_pointer_input_event_get_modifiers(pev));
    EXPECT_EQ(action, mir_pointer_input_event_get_action(pev));
    EXPECT_TRUE(mir_pointer_input_event_get_button_state(pev, mir_pointer_input_button_back));
    EXPECT_TRUE(mir_pointer_input_event_get_button_state(pev, mir_pointer_input_button_tertiary));
    EXPECT_FALSE(mir_pointer_input_event_get_button_state(pev, mir_pointer_input_button_primary));
    EXPECT_FALSE(mir_pointer_input_event_get_button_state(pev, mir_pointer_input_button_secondary));
    EXPECT_FALSE(mir_pointer_input_event_get_button_state(pev, mir_pointer_input_button_forward));
    EXPECT_EQ(x_axis_value, mir_pointer_input_event_get_axis_value(pev, mir_pointer_input_axis_x));
    EXPECT_EQ(y_axis_value, mir_pointer_input_event_get_axis_value(pev, mir_pointer_input_axis_y));
    EXPECT_EQ(hscroll_value, mir_pointer_input_event_get_axis_value(pev, mir_pointer_input_axis_hscroll));
    EXPECT_EQ(vscroll_value, mir_pointer_input_event_get_axis_value(pev, mir_pointer_input_axis_vscroll));
}
