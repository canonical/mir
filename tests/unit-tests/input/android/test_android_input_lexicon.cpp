/*
 * Copyright © 2012 Canonical Ltd.
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

#include "mir/input/android/android_input_lexicon.h"

#include <androidfw/Input.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mi = mir::input;
namespace mia = mir::input::android;

TEST(AndroidInputLexicon, translates_key_events)
{
    using namespace ::testing;
    auto android_key_ev = new android::KeyEvent();

    const int32_t device_id = 1;
    const int32_t source_id = AINPUT_SOURCE_KEYBOARD;
    const int32_t action = AKEY_EVENT_ACTION_DOWN;
    const int32_t flags = 4;
    const int32_t key_code = 5;
    const int32_t scan_code = 6;
    const int32_t meta_state = AMETA_ALT_ON;
    const int32_t repeat_count = 0;
    const uint64_t mac = 11;
    auto const down_time = std::chrono::nanoseconds(9);
    auto const event_time = std::chrono::nanoseconds(10);

    android_key_ev->initialize(device_id, source_id, action, flags, key_code,
                               scan_code, meta_state, repeat_count,
                               mac, down_time, event_time);

    auto mir_ev = mia::Lexicon::translate(android_key_ev);

    EXPECT_EQ(mir_event_type_input, mir_event_get_type(mir_ev.get()));
    auto iev = mir_event_get_input_event(mir_ev.get());
    EXPECT_EQ(device_id, mir_input_event_get_device_id(iev));
    EXPECT_EQ(event_time.count(), mir_input_event_get_event_time(iev));

    EXPECT_EQ(mir_input_event_type_key, mir_input_event_get_type(iev));
    auto kev = mir_input_event_get_keyboard_event(iev);
    EXPECT_EQ(mir_keyboard_action_down, mir_keyboard_event_action(kev));
    EXPECT_EQ(mir_input_event_modifier_alt, mir_keyboard_event_modifiers(kev));

    EXPECT_EQ(key_code, mir_keyboard_event_key_code(kev));
    EXPECT_EQ(scan_code, mir_keyboard_event_scan_code(kev));

    delete android_key_ev;
}

TEST(AndroidInputLexicon, translates_single_pointer_motion_events)
{
    using namespace ::testing;
    auto android_motion_ev = new android::MotionEvent;

    // Common event properties
    const int32_t device_id = 1;
    const int32_t source_id = AINPUT_SOURCE_TOUCHSCREEN;
    const int32_t action = AMOTION_EVENT_ACTION_MOVE;
    const int32_t flags = 4;
    const int32_t edge_flags = 5;
    const int32_t meta_state = 6;
    const int32_t button_state = 0;
    const float x_offset = 8;
    const float y_offset = 9;
    const float x_precision = 10;
    const float y_precision = 11;
    const uint64_t mac = 14;
    auto const down_time = std::chrono::nanoseconds(12);
    auto const event_time = std::chrono::nanoseconds(13);
    const size_t pointer_count = 1;

    // Pointer specific properties (i.e. per touch)
    const int pointer_id = 0;
    droidinput::PointerProperties pointer_properties;
    pointer_properties.id = pointer_id;

    droidinput::PointerCoords pointer_coords;
    pointer_coords.clear();
    const float x_axis = 100.0;
    const float y_axis = 200.0;
    const float touch_minor = 300.0;
    const float touch_major = 400.0;
    const float size = 500.0;
    const float pressure = 600.0;
    const float orientation = 700.0;

    pointer_coords.setAxisValue(AMOTION_EVENT_AXIS_X, x_axis);
    pointer_coords.setAxisValue(AMOTION_EVENT_AXIS_Y, y_axis);
    pointer_coords.setAxisValue(AMOTION_EVENT_AXIS_TOUCH_MAJOR, touch_major);
    pointer_coords.setAxisValue(AMOTION_EVENT_AXIS_TOUCH_MINOR, touch_minor);
    pointer_coords.setAxisValue(AMOTION_EVENT_AXIS_SIZE, size);
    pointer_coords.setAxisValue(AMOTION_EVENT_AXIS_PRESSURE, pressure);
    pointer_coords.setAxisValue(AMOTION_EVENT_AXIS_ORIENTATION, orientation);
    pointer_properties.toolType = AMOTION_EVENT_TOOL_TYPE_UNKNOWN;

    android_motion_ev->initialize(device_id, source_id, action, flags, edge_flags,
                                  meta_state, button_state, x_offset, y_offset,
                                  x_precision, y_precision, mac, down_time,
                                  event_time, pointer_count, &pointer_properties, &pointer_coords);

    auto mir_ev = mia::Lexicon::translate(android_motion_ev);

    EXPECT_EQ(mir_event_type_input, mir_event_get_type(mir_ev.get()));
    auto iev = mir_event_get_input_event(mir_ev.get());

    // Common event properties
    EXPECT_EQ(device_id, mir_input_event_get_device_id(iev));
    EXPECT_EQ(event_time.count(), mir_input_event_get_event_time(iev));
    EXPECT_EQ(mir_input_event_type_touch, mir_input_event_get_type(iev));

    auto tev = mir_input_event_get_touch_event(iev);
    EXPECT_EQ(pointer_count, mir_touch_event_point_count(tev));
    
    EXPECT_EQ(pointer_id, mir_touch_event_id(tev, 0));
    // Notice these two coordinates are offset by x/y offset
    EXPECT_EQ(x_axis + x_offset, mir_touch_event_axis_value(tev, 0, mir_touch_axis_x));
    EXPECT_EQ(y_axis + y_offset, mir_touch_event_axis_value(tev, 0, mir_touch_axis_y));
    EXPECT_EQ(touch_major, mir_touch_event_axis_value(tev, 0, mir_touch_axis_touch_major));
    EXPECT_EQ(touch_minor, mir_touch_event_axis_value(tev, 0, mir_touch_axis_touch_minor));
    EXPECT_EQ(size, mir_touch_event_axis_value(tev, 0, mir_touch_axis_size));
    EXPECT_EQ(pressure, mir_touch_event_axis_value(tev, 0, mir_touch_axis_pressure));


    delete android_motion_ev;
}

TEST(AndroidInputLexicon, translates_multi_pointer_motion_events)
{
    using namespace ::testing;
    auto android_motion_ev = new android::MotionEvent;

    // Common event properties
    const int32_t device_id = 1;
    const int32_t source_id = AINPUT_SOURCE_TOUCHSCREEN;
    const int32_t action = 3 | (2 << AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT);
    const int32_t flags = 4;
    const int32_t edge_flags = 5;
    const int32_t meta_state = 6;
    const float x_offset = 8;
    const float y_offset = 9;
    const float x_precision = 10;
    const float y_precision = 11;
    const uint64_t mac = 14;
    const std::chrono::nanoseconds down_time = std::chrono::nanoseconds(12);
    const std::chrono::nanoseconds event_time = std::chrono::nanoseconds(13);
    const size_t pointer_count = 2;

    const int pointer_id[2] = {1, 2};
    droidinput::PointerProperties pointer_properties[2];
    droidinput::PointerCoords pointer_coords[2];

    pointer_coords[0].clear();
    pointer_coords[1].clear();

    const float x_axis[2] = {100.0, 1000.0};
    const float y_axis[2] = {200.0, 2000.0};
    const float touch_minor[2] = {300.0, 3000.0};
    const float touch_major[2] = {400.0, 4000.0};
    const float size[2] = {500.0, 5000.0};
    const float pressure[2] = {600.0, 6000.0};
    const float orientation[2] = {700.0, 7000.0};

    for (size_t p = 0; p < pointer_count; p++)
    {
        pointer_properties[p].id = pointer_id[p];

        pointer_coords[p].setAxisValue(AMOTION_EVENT_AXIS_X, x_axis[p]);
        pointer_coords[p].setAxisValue(AMOTION_EVENT_AXIS_Y, y_axis[p]);
        pointer_coords[p].setAxisValue(AMOTION_EVENT_AXIS_TOUCH_MAJOR,
                                       touch_major[p]);
        pointer_coords[p].setAxisValue(AMOTION_EVENT_AXIS_TOUCH_MINOR,
                                       touch_minor[p]);
        pointer_coords[p].setAxisValue(AMOTION_EVENT_AXIS_SIZE, size[p]);
        pointer_coords[p].setAxisValue(AMOTION_EVENT_AXIS_PRESSURE,
                                       pressure[p]);
        pointer_coords[p].setAxisValue(AMOTION_EVENT_AXIS_ORIENTATION,
                                       orientation[p]);

        pointer_properties[p].toolType = AMOTION_EVENT_TOOL_TYPE_UNKNOWN;
    }

    android_motion_ev->initialize(device_id, source_id, action, flags,
                                  edge_flags, meta_state, 0,
                                  x_offset, y_offset, x_precision, y_precision,
                                  mac, down_time, event_time, pointer_count,
                                  pointer_properties, pointer_coords);

    auto mir_ev = mia::Lexicon::translate(android_motion_ev);

    EXPECT_EQ(mir_event_type_input, mir_event_get_type(mir_ev.get()));
    auto iev = mir_event_get_input_event(mir_ev.get());

    // Common event properties
    EXPECT_EQ(device_id, mir_input_event_get_device_id(iev));
    EXPECT_EQ(event_time.count(), mir_input_event_get_event_time(iev));
    EXPECT_EQ(mir_input_event_type_touch, mir_input_event_get_type(iev));

    auto tev = mir_input_event_get_touch_event(iev);
    EXPECT_EQ(pointer_count, mir_touch_event_point_count(tev));

    for (unsigned i = 0; i < pointer_count; i++)
    {
        EXPECT_EQ(pointer_id[i], mir_touch_event_id(tev, i));
        EXPECT_EQ(x_axis[i] + x_offset, mir_touch_event_axis_value(tev, i, mir_touch_axis_x));
        EXPECT_EQ(y_axis[i] + y_offset, mir_touch_event_axis_value(tev, i, mir_touch_axis_y));
        EXPECT_EQ(touch_major[i], mir_touch_event_axis_value(tev, i, mir_touch_axis_touch_major));
        EXPECT_EQ(touch_minor[i], mir_touch_event_axis_value(tev, i, mir_touch_axis_touch_minor));
        EXPECT_EQ(size[i], mir_touch_event_axis_value(tev, i, mir_touch_axis_size));
        EXPECT_EQ(pressure[i], mir_touch_event_axis_value(tev, i, mir_touch_axis_pressure));
    }

    delete android_motion_ev;
}
