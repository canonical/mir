/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
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

#include "mir/events/event_builders.h"

#include "src/server/input/android/android_input_dispatcher.h"
#include "src/server/input/android/android_input_thread.h"
#include "src/server/input/android/android_input_constants.h"

#include "mir_test/fake_shared.h"
#include "mir_test_doubles/mock_android_input_dispatcher.h"

#include <utils/StrongPointer.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <initializer_list>
#include <cstring>

namespace droidinput = android;

namespace mi = mir::input;
namespace mia = mir::input::android;
namespace mev = mir::events;
namespace mt = mir::test;
namespace mtd = mt::doubles;

// Mock objects
namespace
{

struct MockInputThread : public mia::InputThread
{
    MOCK_METHOD0(start, void());
    MOCK_METHOD0(request_stop, void());
    MOCK_METHOD0(join, void());
};

}

// Test fixture
namespace
{

struct AndroidInputDispatcherTest : public testing::Test
{
    ~AndroidInputDispatcherTest()
    {
        using namespace ::testing;

        Mock::VerifyAndClearExpectations(dispatcher.get());
        Mock::VerifyAndClearExpectations(dispatcher_thread.get());

        // mia::AndroidInputDispatcher destruction expectations
        InSequence s;
        EXPECT_CALL(*dispatcher_thread, request_stop());
        EXPECT_CALL(*dispatcher, setInputDispatchMode(mia::DispatchDisabled, mia::DispatchFrozen));
        EXPECT_CALL(*dispatcher_thread, join());
    }

    std::shared_ptr<mtd::MockAndroidInputDispatcher> dispatcher = std::make_shared<mtd::MockAndroidInputDispatcher>();
    std::shared_ptr<MockInputThread> dispatcher_thread = std::make_shared<MockInputThread>();
    mia::AndroidInputDispatcher input_dispatcher{dispatcher,dispatcher_thread};
    const uint32_t default_policy_flags = 0;

    int32_t const device_id = 1;
    int32_t const touch_id = 2;
    std::chrono::nanoseconds const timestamp = std::chrono::nanoseconds(2);
};

}

TEST_F(AndroidInputDispatcherTest, start_starts_dispatcher_and_thread)
{
    using namespace ::testing;

    InSequence seq;

    EXPECT_CALL(*dispatcher, setInputDispatchMode(mia::DispatchEnabled, mia::DispatchUnfrozen))
        .Times(1);
    EXPECT_CALL(*dispatcher, setInputFilterEnabled(true))
        .Times(1);

    EXPECT_CALL(*dispatcher_thread, start())
        .Times(1);

    input_dispatcher.start();
}

TEST_F(AndroidInputDispatcherTest, stop_stops_dispatcher_and_thread)
{
    using namespace ::testing;

    InSequence seq;

    EXPECT_CALL(*dispatcher_thread, request_stop());
    EXPECT_CALL(*dispatcher, setInputDispatchMode(mia::DispatchDisabled, mia::DispatchFrozen)).Times(1);
    EXPECT_CALL(*dispatcher_thread, join());

    input_dispatcher.stop();
}

MATCHER_P(MotionArgsMatches, expected, "")
{
    bool scalar_members_identical =
        arg->eventTime == expected.eventTime &&
        arg->deviceId == expected.deviceId &&
        arg->action == expected.action &&
        arg->flags == expected.flags &&
        arg->metaState == expected.metaState &&
        arg->buttonState == expected.buttonState &&
        arg->edgeFlags == expected.edgeFlags &&
        arg->pointerCount == expected.pointerCount &&
        arg->xPrecision == expected.xPrecision &&
        arg->yPrecision == expected.yPrecision &&
        arg->downTime == expected.downTime;

    if (!scalar_members_identical)
        return false;

    int relevant_axis[] = {AMOTION_EVENT_AXIS_ORIENTATION, AMOTION_EVENT_AXIS_TOUCH_MAJOR, AMOTION_EVENT_AXIS_PRESSURE,
                           AMOTION_EVENT_AXIS_X,           AMOTION_EVENT_AXIS_Y,           AMOTION_EVENT_AXIS_SIZE,
                           AMOTION_EVENT_AXIS_HSCROLL,     AMOTION_EVENT_AXIS_VSCROLL};
    for (uint32_t i = 0; i != arg->pointerCount; ++i)
    {
        for (int j = 0; j != sizeof(relevant_axis)/sizeof(relevant_axis[0]); ++j)
        {
            float expected_axis = expected.pointerCoords[i].getAxisValue(relevant_axis[j]);
            float received_axis = arg->pointerCoords[i].getAxisValue(relevant_axis[j]);
            if (expected_axis != received_axis)
                return false;
        }
        if (expected.pointerProperties[i].id != arg->pointerProperties[i].id ||
            expected.pointerProperties[i].toolType!= arg->pointerProperties[i].toolType)
        {
            return false;
        }
    }

    return true;
}

MATCHER_P(KeyArgsMatches, expected, "")
{
    return
        arg->eventTime == expected.eventTime &&
        arg->deviceId == expected.deviceId &&
        arg->source == expected.source &&
        arg->action == expected.action &&
        arg->flags == expected.flags &&
        arg->keyCode == expected.keyCode &&
        arg->scanCode == expected.scanCode &&
        arg->metaState == expected.metaState &&
        arg->downTime == expected.downTime;
}

TEST_F(AndroidInputDispatcherTest, axis_values_are_properly_converted)
{
    using namespace ::testing;
    droidinput::PointerCoords expected_coords[1];
    droidinput::PointerProperties expected_properties[1];

    std::memset(expected_coords, 0, sizeof(expected_coords));
    std::memset(expected_properties, 0, sizeof(expected_properties));

    float x = 12.0f, y = 13.0f, touch_major = 14.0f, touch_minor = 15.0f,
        size = 16.0f, pressure = 17.0f;
    
    auto event = mev::make_event(MirInputDeviceId(device_id), timestamp, mir_input_event_modifier_shift);

    mev::add_touch(*event, MirTouchId(touch_id), mir_touch_action_change,
                   mir_touch_tooltype_finger, x, y, pressure, touch_major, touch_minor, size);

    expected_coords[0].setAxisValue(AMOTION_EVENT_AXIS_X, x);
    expected_coords[0].setAxisValue(AMOTION_EVENT_AXIS_Y, y);
    expected_coords[0].setAxisValue(AMOTION_EVENT_AXIS_TOUCH_MAJOR, touch_major);
    expected_coords[0].setAxisValue(AMOTION_EVENT_AXIS_TOUCH_MINOR, touch_minor);
    expected_coords[0].setAxisValue(AMOTION_EVENT_AXIS_SIZE, size);
    expected_coords[0].setAxisValue(AMOTION_EVENT_AXIS_PRESSURE, pressure);
    expected_properties[0].id = touch_id;
    expected_properties[0].toolType = AMOTION_EVENT_TOOL_TYPE_FINGER;

    droidinput::NotifyMotionArgs expected(timestamp,
                                          device_id,
                                          AINPUT_SOURCE_TOUCHSCREEN,
                                          default_policy_flags,
                                          AMOTION_EVENT_ACTION_MOVE,
                                          0, /* flags */
                                          AMETA_SHIFT_ON,
                                          0,
                                          0, /* edge_flags */
                                          1,
                                          expected_properties,
                                          expected_coords,
                                          0, 0, /* unused x/y precision */
                                          timestamp);

    EXPECT_CALL(*dispatcher, notifyMotion(MotionArgsMatches(expected)));

    input_dispatcher.dispatch(*event);
}

TEST_F(AndroidInputDispatcherTest, forwards_all_key_event_paramters_correctly)
{
    using namespace ::testing;

    xkb_keysym_t key_code = 5;
    int scan_code = 6;
    auto event = mev::make_event(MirInputDeviceId(device_id), timestamp, mir_keyboard_action_down, key_code, scan_code,
                                 mir_input_event_modifier_shift);

    droidinput::NotifyKeyArgs expected(timestamp,
                                       device_id,
                                       AINPUT_SOURCE_KEYBOARD,
                                       default_policy_flags,
                                       AKEY_EVENT_ACTION_DOWN,
                                       0, /* flags */
                                       key_code,
                                       scan_code,
                                       AMETA_SHIFT_ON,
                                       timestamp);

    EXPECT_CALL(*dispatcher, notifyKey(KeyArgsMatches(expected)));

    input_dispatcher.dispatch(*event);
}
