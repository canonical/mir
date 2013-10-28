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
#include "mir_toolkit/event.h"

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
    const int32_t source_id = 2;
    const int32_t action = 3;
    const int32_t flags = 4;
    const int32_t key_code = 5;
    const int32_t scan_code = 6;
    const int32_t meta_state = 7;
    const int32_t repeat_count = 8;
    const nsecs_t down_time = 9;
    const nsecs_t event_time = 10;

    android_key_ev->initialize(device_id, source_id, action, flags, key_code,
                               scan_code, meta_state, repeat_count,
                               down_time, event_time);

    MirEvent mir_ev;
    mia::Lexicon::translate(android_key_ev, mir_ev);

    // Common event properties
    EXPECT_EQ(device_id, mir_ev.key.device_id);
    EXPECT_EQ(source_id, mir_ev.key.source_id);
    EXPECT_EQ(action, mir_ev.key.action);
    EXPECT_EQ(flags, mir_ev.key.flags);
    EXPECT_EQ((unsigned int)meta_state, mir_ev.key.modifiers);

    auto mir_key_ev = &mir_ev.key;
    // Key event specific properties
    EXPECT_EQ(mir_ev.type, mir_event_type_key);
    EXPECT_EQ(mir_key_ev->key_code, key_code);
    EXPECT_EQ(mir_key_ev->scan_code, scan_code);
    EXPECT_EQ(mir_key_ev->repeat_count, repeat_count);
    EXPECT_EQ(mir_key_ev->down_time, down_time);
    EXPECT_EQ(mir_key_ev->event_time, event_time);
    // What is this flag and where does it come from?
    EXPECT_EQ(mir_key_ev->is_system_key, false);

    delete android_key_ev;
}

TEST(AndroidInputLexicon, translates_single_pointer_motion_events)
{
    using namespace ::testing;
    auto android_motion_ev = new android::MotionEvent;

    // Common event properties
    const int32_t device_id = 1;
    const int32_t source_id = 2;
    const int32_t action = 3;
    const int32_t flags = 4;
    const int32_t edge_flags = 5;
    const int32_t meta_state = 6;
    const int32_t button_state = 7;
    const float x_offset = 8;
    const float y_offset = 9;
    const float x_precision = 10;
    const float y_precision = 11;
    const nsecs_t down_time = 12;
    const nsecs_t event_time = 13;
    const size_t pointer_count = 1;

    // Pointer specific properties (i.e. per touch)
    const int pointer_id = 1;
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
    const float vscroll = 800.0;
    const float hscroll = 900.0;

    pointer_coords.setAxisValue(AMOTION_EVENT_AXIS_X, x_axis);
    pointer_coords.setAxisValue(AMOTION_EVENT_AXIS_Y, y_axis);
    pointer_coords.setAxisValue(AMOTION_EVENT_AXIS_TOUCH_MAJOR, touch_major);
    pointer_coords.setAxisValue(AMOTION_EVENT_AXIS_TOUCH_MINOR, touch_minor);
    pointer_coords.setAxisValue(AMOTION_EVENT_AXIS_SIZE, size);
    pointer_coords.setAxisValue(AMOTION_EVENT_AXIS_PRESSURE, pressure);
    pointer_coords.setAxisValue(AMOTION_EVENT_AXIS_ORIENTATION, orientation);
    pointer_coords.setAxisValue(AMOTION_EVENT_AXIS_VSCROLL, vscroll);
    pointer_coords.setAxisValue(AMOTION_EVENT_AXIS_HSCROLL, hscroll);

    android_motion_ev->initialize(device_id, source_id, action, flags, edge_flags,
                                  meta_state, button_state, x_offset, y_offset,
                                  x_precision, y_precision, down_time,
                                  event_time, pointer_count, &pointer_properties, &pointer_coords);

    MirEvent mir_ev;
    mia::Lexicon::translate(android_motion_ev, mir_ev);

    // Common event properties
    EXPECT_EQ(device_id, mir_ev.motion.device_id);
    EXPECT_EQ(source_id, mir_ev.motion.source_id);
    EXPECT_EQ(action, mir_ev.motion.action);
    EXPECT_EQ(flags, mir_ev.motion.flags);
    EXPECT_EQ((unsigned int)meta_state, mir_ev.motion.modifiers);

    // Motion event specific properties
    EXPECT_EQ(mir_ev.type, mir_event_type_motion);

    auto mir_motion_ev = &mir_ev.motion;

    EXPECT_EQ(mir_motion_ev->edge_flags, edge_flags);
    EXPECT_EQ(mir_motion_ev->button_state, button_state);
    EXPECT_EQ(mir_motion_ev->x_offset, x_offset);
    EXPECT_EQ(mir_motion_ev->y_offset, y_offset);
    EXPECT_EQ(mir_motion_ev->x_precision, x_precision);
    EXPECT_EQ(mir_motion_ev->y_precision, y_precision);
    EXPECT_EQ(mir_motion_ev->down_time, down_time);
    EXPECT_EQ(mir_motion_ev->event_time, event_time);

    EXPECT_EQ(mir_motion_ev->pointer_count, pointer_count);

    auto mir_pointer_coords = &mir_motion_ev->pointer_coordinates[0];

    EXPECT_EQ(mir_pointer_coords->id, pointer_id);
    // Notice these two coordinates are offset by x/y offset
    EXPECT_EQ(mir_pointer_coords->x, x_axis + x_offset);
    EXPECT_EQ(mir_pointer_coords->y, y_axis + y_offset);
    EXPECT_EQ(mir_pointer_coords->raw_x, x_axis);
    EXPECT_EQ(mir_pointer_coords->raw_y, y_axis);
    EXPECT_EQ(mir_pointer_coords->touch_major, touch_major);
    EXPECT_EQ(mir_pointer_coords->touch_minor, touch_minor);
    EXPECT_EQ(mir_pointer_coords->size, size);
    EXPECT_EQ(mir_pointer_coords->pressure, pressure);
    EXPECT_EQ(mir_pointer_coords->orientation, orientation);
    EXPECT_EQ(mir_pointer_coords->vscroll, vscroll);
    EXPECT_EQ(mir_pointer_coords->hscroll, hscroll);


    delete android_motion_ev;
}

TEST(AndroidInputLexicon, translates_multi_pointer_motion_events)
{
    using namespace ::testing;
    auto android_motion_ev = new android::MotionEvent;

    // Common event properties
    const int32_t device_id = 1;
    const int32_t source_id = 2;
    const int32_t action = 3 | (2 << AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT);
    const int32_t flags = 4;
    const int32_t edge_flags = 5;
    const int32_t meta_state = 6;
    const int32_t button_state = 7;
    const float x_offset = 8;
    const float y_offset = 9;
    const float x_precision = 10;
    const float y_precision = 11;
    const nsecs_t down_time = 12;
    const nsecs_t event_time = 13;
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
    const float vscroll[2] = {800.0, 8000.0};
    const float hscroll[2] = {900.0, 9000.0};

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
        pointer_coords[p].setAxisValue(AMOTION_EVENT_AXIS_VSCROLL,
                                       vscroll[p]);
        pointer_coords[p].setAxisValue(AMOTION_EVENT_AXIS_HSCROLL,
                                       hscroll[p]);
    }

    android_motion_ev->initialize(device_id, source_id, action, flags,
                                  edge_flags, meta_state, button_state,
                                  x_offset, y_offset, x_precision, y_precision,
                                  down_time, event_time, pointer_count,
                                  pointer_properties, pointer_coords);

    MirEvent mir_ev;
    mia::Lexicon::translate(android_motion_ev, mir_ev);

    // Common event properties
    EXPECT_EQ(device_id, mir_ev.motion.device_id);
    EXPECT_EQ(source_id, mir_ev.motion.source_id);
    EXPECT_EQ(action, mir_ev.motion.action);
    EXPECT_EQ(flags, mir_ev.motion.flags);
    EXPECT_EQ((unsigned int)meta_state, mir_ev.motion.modifiers);

    // Motion event specific properties
    EXPECT_EQ(mir_ev.type, mir_event_type_motion);

    auto mir_motion_ev = &mir_ev.motion;

    EXPECT_EQ(mir_motion_ev->edge_flags, edge_flags);
    EXPECT_EQ(mir_motion_ev->button_state, button_state);
    EXPECT_EQ(mir_motion_ev->x_offset, x_offset);
    EXPECT_EQ(mir_motion_ev->y_offset, y_offset);
    EXPECT_EQ(mir_motion_ev->x_precision, x_precision);
    EXPECT_EQ(mir_motion_ev->y_precision, y_precision);
    EXPECT_EQ(mir_motion_ev->down_time, down_time);
    EXPECT_EQ(mir_motion_ev->event_time, event_time);
    EXPECT_EQ(mir_motion_ev->pointer_count, pointer_count);

    auto pointer = &mir_motion_ev->pointer_coordinates[0];

    for (size_t p = 0; p < pointer_count; p++)
    {
        EXPECT_EQ(pointer[p].id, pointer_id[p]);
        EXPECT_EQ(pointer[p].x, x_axis[p] + x_offset);
        EXPECT_EQ(pointer[p].y, y_axis[p] + y_offset);
        EXPECT_EQ(pointer[p].raw_x, x_axis[p]);
        EXPECT_EQ(pointer[p].raw_y, y_axis[p]);
        EXPECT_EQ(pointer[p].touch_major, touch_major[p]);
        EXPECT_EQ(pointer[p].touch_minor, touch_minor[p]);
        EXPECT_EQ(pointer[p].size, size[p]);
        EXPECT_EQ(pointer[p].pressure, pressure[p]);
        EXPECT_EQ(pointer[p].orientation, orientation[p]);
        EXPECT_EQ(pointer[p].vscroll, vscroll[p]);
        EXPECT_EQ(pointer[p].hscroll, hscroll[p]);
    }

    delete android_motion_ev;
}
