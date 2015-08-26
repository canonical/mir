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
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
 */


#include "src/server/input/android/input_translator.h"
#include "mir/events/event_private.h"
#include "mir/events/event_builders.h"

#include "mir/test/doubles/mock_input_dispatcher.h"
#include "mir/test/fake_shared.h"
#include "mir/test/event_matchers.h"

#include "InputListener.h"
#include "androidfw/Input.h"

#include <cstring>

namespace droidinput = android;

namespace mev = mir::events;
namespace mia = mir::input::android;
namespace mt = mir::test;
namespace mtd = mt::doubles;

namespace
{
class InputTranslator : public ::testing::Test
{
public:
    ::testing::NiceMock<mtd::MockInputDispatcher> dispatcher;
    mia::InputTranslator translator;

    InputTranslator()
        : translator(mt::fake_shared(dispatcher))
    {
        std::memset(coords, 0, sizeof(coords));
        std::memset(properties, 0, sizeof(properties));
    }
    std::chrono::nanoseconds const some_time = std::chrono::nanoseconds(13);
    std::chrono::nanoseconds const later_time = std::chrono::nanoseconds(14);
    const uint64_t mac = 16;
    const int32_t device_id = 13;
    const uint32_t source_id = 13;
    const uint32_t default_flags = 0;
    const uint32_t default_policy_flags = 0;
    const int32_t no_flags= 0;
    const int32_t no_modifiers = 0;
    const int32_t arbitrary_key_code = 17;
    const int32_t arbitrary_scan_code = 17;
    const uint32_t zero_pointers = 0;
    const float x_precision = 2.0f;
    const float y_precision = 2.0f;
    const int32_t meta_state = 0;
    const int32_t edge_flags = 0;
    const int32_t button_state = 0;
    const int32_t motion_action = AMOTION_EVENT_ACTION_MOVE;
    droidinput::PointerCoords coords[MIR_INPUT_EVENT_MAX_POINTER_COUNT];
    droidinput::PointerProperties properties[MIR_INPUT_EVENT_MAX_POINTER_COUNT];
};

struct PolicyFlagTestParameter
{
    const uint32_t policy_flag;
    const unsigned int expected_modifiers;
};

class InputTranslatorWithPolicyParam : public InputTranslator,
    public ::testing::WithParamInterface<PolicyFlagTestParameter>
{
};

}

TEST_F(InputTranslator, notifies_configuration_change)
{
    using namespace ::testing;

    EXPECT_CALL(dispatcher, dispatch(mt::InputDeviceConfigurationChangedEvent())).Times(1);

    droidinput::NotifyConfigurationChangedArgs change(some_time);
    translator.notifyConfigurationChanged(&change);
}

TEST_F(InputTranslator, notifies_device_reset)
{
    using namespace ::testing;

    EXPECT_CALL(dispatcher, dispatch(mt::InputDeviceResetEvent())).Times(1);

    droidinput::NotifyDeviceResetArgs reset(later_time, device_id);
    translator.notifyDeviceReset(&reset);
}

TEST_F(InputTranslator, accepts_motion_action_with_existing_index)
{
    using namespace ::testing;

    EXPECT_CALL(dispatcher, dispatch(_)).Times(1);

    const int32_t valid_motion_action = AMOTION_EVENT_ACTION_POINTER_UP | (2 << AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT);
    const uint32_t three_pointers = 3;

    properties[0].id = 1;
    properties[1].id = 2;
    properties[2].id = 3;

    droidinput::NotifyMotionArgs motion(some_time, mac, device_id, source_id, 0, valid_motion_action,
                                        no_flags, meta_state, button_state, edge_flags, three_pointers, properties,
                                        coords, x_precision, y_precision, later_time);

    translator.notifyMotion(&motion);
}

MATCHER(EndOfTouchGesture, "")
{
    MirEvent const& ev = arg;
    if (mir_event_get_type(&ev) != mir_event_type_input)
        return false;

    auto iev = mir_event_get_input_event(&ev);
    if (mir_input_event_get_type(iev) != mir_input_event_type_touch)
        return false;

    auto tev = mir_input_event_get_touch_event(iev);
    if (mir_touch_event_point_count(tev) < 1)
        return false;

    return mir_touch_event_action(tev, 0) == mir_touch_action_up;
}

TEST_F(InputTranslator, translates_multifinger_release_correctly)
{   // Regression test for LP: #1481570
    using namespace ::testing;

    EXPECT_CALL(dispatcher, dispatch(EndOfTouchGesture()))
        .Times(1);

    /*
     * Android often provides old indices on AMOTION_EVENT_ACTION_UP.
     * But that doesn't matter at all because AMOTION_EVENT_ACTION_UP means
     * that all fingers were released. (LP: #1481570).
     */
    int32_t const invalid_id = 7;
    int32_t const end_of_gesture = AMOTION_EVENT_ACTION_UP
                    | (invalid_id << AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT);

    /*
     * A multifinger test with only one finger? Yep, that's right. That's
     * what we see in production. A dummy single finger event used to indicate
     * all fingers (regardless of number) were released.
     */
    properties[0].id = 1;
    properties[0].toolType = AMOTION_EVENT_TOOL_TYPE_FINGER;

    droidinput::NotifyMotionArgs motion(
        some_time, mac, device_id, source_id, 0, end_of_gesture,
        no_flags, meta_state, button_state, edge_flags, 1, properties,
        coords, x_precision, y_precision, later_time);

    translator.notifyMotion(&motion);
}

TEST_F(InputTranslator, ignores_motion_with_duplicated_pointerids)
{
    using namespace ::testing;

    EXPECT_CALL(dispatcher, dispatch(_)).Times(0);

    const uint32_t three_pointers = 3;

    properties[0].id = 1;
    properties[1].id = 1;
    properties[2].id = 3;

    droidinput::NotifyMotionArgs motion(some_time, mac, device_id, source_id, 0, motion_action, no_flags,
                                        meta_state, button_state, edge_flags, three_pointers, properties, coords,
                                        x_precision, y_precision, later_time);
    translator.notifyMotion(&motion);
}

TEST_F(InputTranslator, ignores_motion_with_invalid_pointerids)
{
    using namespace ::testing;

    EXPECT_CALL(dispatcher, dispatch(_)).Times(0);

    const uint32_t three_pointers = 3;

    properties[0].id = MAX_POINTER_ID + 1;
    properties[1].id = 1;
    properties[2].id = 3;

    droidinput::NotifyMotionArgs motion(some_time, mac, device_id, source_id, 0, motion_action, no_flags,
                                        meta_state, button_state, edge_flags, three_pointers, properties, coords,
                                        x_precision, y_precision, later_time);
    translator.notifyMotion(&motion);
}

TEST_F(InputTranslator, forwards_pointer_positions)
{
    using namespace ::testing;

    const float x_pos = 12.0f;
    const float y_pos = 30.0f;
    const float dx = 7.0f;
    const float dy = 9.3f;
    EXPECT_CALL(dispatcher, dispatch(AllOf(
        mt::PointerEventWithPosition(x_pos, y_pos),
        mt::PointerEventWithDiff(dx, dy)))).Times(1);

    const uint32_t one_pointer = 1;

    properties[0].id = 23;
    coords[0].setAxisValue(AMOTION_EVENT_AXIS_X, x_pos);
    coords[0].setAxisValue(AMOTION_EVENT_AXIS_Y, y_pos);
    coords[0].setAxisValue(AMOTION_EVENT_AXIS_RX, dx);
    coords[0].setAxisValue(AMOTION_EVENT_AXIS_RY, dy);
    
    droidinput::NotifyMotionArgs motion(some_time, mac, device_id, AINPUT_SOURCE_MOUSE, 0, motion_action, no_flags,
                                        meta_state, button_state, edge_flags, one_pointer, properties, coords,
                                        x_precision, y_precision, later_time);
    translator.notifyMotion(&motion);
}

TEST_F(InputTranslator, forwards_and_converts_up_down_key_notifications)
{
    using namespace ::testing;

    InSequence seq;
    EXPECT_CALL(dispatcher, dispatch(mt::KeyDownEvent())).Times(1);
    EXPECT_CALL(dispatcher, dispatch(mt::KeyUpEvent())).Times(1);

    droidinput::NotifyKeyArgs down(some_time, mac, device_id, source_id, 0, AKEY_EVENT_ACTION_DOWN,
                                   no_flags, arbitrary_key_code, arbitrary_scan_code, no_modifiers, later_time);
    droidinput::NotifyKeyArgs up(some_time, mac, device_id, source_id, 0, AKEY_EVENT_ACTION_UP,
                                 no_flags, arbitrary_key_code, arbitrary_scan_code, no_modifiers, later_time);

    translator.notifyKey(&down);
    translator.notifyKey(&up);
}

TEST_F(InputTranslator, forwards_all_key_event_paramters_correctly)
{
    using namespace ::testing;

    int32_t const device_id = 2, scan_code = 4, key_code = 5;
    std::chrono::nanoseconds event_time(1);
    auto mac = 0;

    auto expected = mev::make_event(MirInputDeviceId(device_id), event_time, mac,
                                    mir_keyboard_action_down, key_code, scan_code,
                                    mir_input_event_modifier_shift);

    InSequence seq;
    EXPECT_CALL(dispatcher, dispatch(mt::MirKeyEventMatches(*expected))).Times(1);

    droidinput::NotifyKeyArgs notified(event_time,
                                       mac,
                                       device_id,
                                       AINPUT_SOURCE_KEYBOARD,
                                       default_policy_flags,
                                       AKEY_EVENT_ACTION_DOWN,
                                       0, /* flags */
                                       key_code,
                                       scan_code,
                                       AMETA_SHIFT_ON,
                                       event_time);

    translator.notifyKey(&notified);
}

TEST_F(InputTranslator, forwards_all_motion_event_paramters_correctly)
{
    using namespace ::testing;

    std::chrono::nanoseconds event_time(2);
    auto mac = 0;
    int32_t device_id = 3;
    int32_t touch_id = 17;
    float x = 7, y = 8, pres = 9, tmaj = 10, tmin = 11, size = 12;

    auto expected = mev::make_event(MirInputDeviceId(device_id), event_time, mac, mir_input_event_modifier_none);
    mev::add_touch(*expected,  MirTouchId(touch_id), mir_touch_action_change,
                   mir_touch_tooltype_finger, x, y, pres, tmaj, tmin, size);

    coords[0].setAxisValue(AMOTION_EVENT_AXIS_X, x);
    coords[0].setAxisValue(AMOTION_EVENT_AXIS_Y, y);
    coords[0].setAxisValue(AMOTION_EVENT_AXIS_TOUCH_MAJOR, tmaj);
    coords[0].setAxisValue(AMOTION_EVENT_AXIS_TOUCH_MINOR, tmin);
    coords[0].setAxisValue(AMOTION_EVENT_AXIS_SIZE, size);
    coords[0].setAxisValue(AMOTION_EVENT_AXIS_PRESSURE, pres);
    properties[0].id = touch_id;
    properties[0].toolType = AMOTION_EVENT_TOOL_TYPE_FINGER;
    InSequence seq;
    EXPECT_CALL(dispatcher, dispatch(mt::MirTouchEventMatches(*expected))).Times(1);

    droidinput::NotifyMotionArgs notified(std::chrono::nanoseconds(event_time),
                                          mac,
                                          device_id,
                                          AINPUT_SOURCE_TOUCHSCREEN,
                                          default_policy_flags,
                                          AMOTION_EVENT_ACTION_HOVER_MOVE,
                                          0, /* flags */
                                          0,
                                          0,
                                          0, /* edge flags */
                                          1,
                                          properties,
                                          coords,
                                          0, 0, /* unused x/y precision */
                                          event_time);

    translator.notifyMotion(&notified);
}

TEST_P(InputTranslatorWithPolicyParam, forwards_policy_modifiers_as_flags_and_modifiers)
{
    using namespace ::testing;

    EXPECT_CALL(dispatcher,
                dispatch(
                        mt::KeyWithModifiers(GetParam().expected_modifiers)
                    )
                ).Times(1);

    droidinput::NotifyKeyArgs tester(some_time, mac, device_id, source_id,
                                     GetParam().policy_flag, AKEY_EVENT_ACTION_DOWN,
                                     no_flags, arbitrary_key_code, arbitrary_scan_code, no_modifiers, later_time);

    translator.notifyKey(&tester);
}

INSTANTIATE_TEST_CASE_P(VariousPolicyFlags, InputTranslatorWithPolicyParam,
                        ::testing::Values(PolicyFlagTestParameter{droidinput::POLICY_FLAG_CAPS_LOCK,
                                                                  mir_input_event_modifier_caps_lock},
                                          PolicyFlagTestParameter{droidinput::POLICY_FLAG_ALT,
                                                                  mir_input_event_modifier_alt | mir_input_event_modifier_alt_left},
                                          PolicyFlagTestParameter{droidinput::POLICY_FLAG_ALT_GR,
                                                                  mir_input_event_modifier_alt | mir_input_event_modifier_alt_right},
                                          PolicyFlagTestParameter{droidinput::POLICY_FLAG_FUNCTION,
                                                                  mir_input_event_modifier_function},
                                          PolicyFlagTestParameter{droidinput::POLICY_FLAG_SHIFT,
                                                                  mir_input_event_modifier_shift | mir_input_event_modifier_shift_left},
                                          PolicyFlagTestParameter{droidinput::POLICY_FLAG_VIRTUAL,
                                                                  mir_input_event_modifier_none}));


