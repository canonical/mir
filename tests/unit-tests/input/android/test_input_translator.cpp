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

#include "mir_test_doubles/mock_input_dispatcher.h"
#include "mir_test/fake_shared.h"
#include "mir_test/event_matchers.h"

#include "InputListener.h"
#include "androidfw/Input.h"

#include <cstring>

namespace droidinput = android;
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
    const int32_t motion_action = mir_motion_action_move;
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

TEST_F(InputTranslator, ignores_invalid_motion_action)
{
    using namespace ::testing;

    EXPECT_CALL(dispatcher, dispatch(_)).Times(0);

    const int32_t invalid_motion_action = 20;

    droidinput::NotifyMotionArgs motion(some_time, device_id, source_id, 0, invalid_motion_action,
                                        no_flags, meta_state, button_state, edge_flags, zero_pointers, properties,
                                        coords, x_precision, y_precision, later_time);

    translator.notifyMotion(&motion);
}

TEST_F(InputTranslator, ignores_motion_action_with_wrong_index)
{
    using namespace ::testing;

    EXPECT_CALL(dispatcher, dispatch(_)).Times(0);

    const int32_t invalid_motion_action = mir_motion_action_pointer_up | (3 << AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT);

    droidinput::NotifyMotionArgs motion(some_time, device_id, source_id, 0, invalid_motion_action,
                                        no_flags, meta_state, button_state, edge_flags, zero_pointers, properties,
                                        coords, x_precision, y_precision, later_time);

    translator.notifyMotion(&motion);
}

TEST_F(InputTranslator, accepts_motion_action_with_existing_index)
{
    using namespace ::testing;

    EXPECT_CALL(dispatcher, dispatch(_)).Times(1);

    const int32_t valid_motion_action = mir_motion_action_pointer_up | (2 << AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT);
    const uint32_t three_pointers = 3;

    properties[0].id = 1;
    properties[1].id = 2;
    properties[2].id = 3;

    droidinput::NotifyMotionArgs motion(some_time, device_id, source_id, 0, valid_motion_action,
                                        no_flags, meta_state, button_state, edge_flags, three_pointers, properties,
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

    droidinput::NotifyMotionArgs motion(some_time, device_id, source_id, 0, motion_action, no_flags,
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

    droidinput::NotifyMotionArgs motion(some_time, device_id, source_id, 0, motion_action, no_flags,
                                        meta_state, button_state, edge_flags, three_pointers, properties, coords,
                                        x_precision, y_precision, later_time);
    translator.notifyMotion(&motion);
}

TEST_F(InputTranslator, forwards_pointer_positions)
{
    using namespace ::testing;

    const float x_pos = 12.0f;
    const float y_pos = 30.0f;
    EXPECT_CALL(dispatcher, dispatch(mt::PointerEventWithPosition(x_pos, y_pos))).Times(1);

    const uint32_t one_pointer = 1;

    properties[0].id = 23;
    coords[0].setAxisValue(AMOTION_EVENT_AXIS_X, x_pos);
    coords[0].setAxisValue(AMOTION_EVENT_AXIS_Y, y_pos);
    
    droidinput::NotifyMotionArgs motion(some_time, device_id, AINPUT_SOURCE_MOUSE, 0, motion_action, no_flags,
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

    droidinput::NotifyKeyArgs down(some_time, device_id, source_id, 0, AKEY_EVENT_ACTION_DOWN,
                                   no_flags, arbitrary_key_code, arbitrary_scan_code, no_modifiers, later_time);
    droidinput::NotifyKeyArgs up(some_time, device_id, source_id, 0, AKEY_EVENT_ACTION_UP,
                                 no_flags, arbitrary_key_code, arbitrary_scan_code, no_modifiers, later_time);

    translator.notifyKey(&down);
    translator.notifyKey(&up);
}

TEST_F(InputTranslator, forwards_all_key_event_paramters_correctly)
{
    using namespace ::testing;
    MirEvent expected;
    expected.type = mir_event_type_key;
    expected.key.event_time = std::chrono::nanoseconds(1);
    expected.key.device_id = 2;
    expected.key.source_id = 3;
    expected.key.action = mir_keyboard_action_down;
    expected.key.scan_code = 4;
    expected.key.key_code = 5;
    expected.key.modifiers = mir_input_event_modifier_shift;

    InSequence seq;
    EXPECT_CALL(dispatcher, dispatch(mt::MirKeyEventMatches(expected))).Times(1);

    droidinput::NotifyKeyArgs notified(std::chrono::nanoseconds(expected.key.event_time),
                                       expected.key.device_id,
                                       expected.key.source_id,
                                       default_policy_flags,
                                       AKEY_EVENT_ACTION_DOWN,
                                       0, /* flags */
                                       expected.key.key_code,
                                       expected.key.scan_code,
                                       AMETA_SHIFT_ON,
                                       expected.key.event_time);

    translator.notifyKey(&notified);
}

TEST_F(InputTranslator, forwards_all_motion_event_paramters_correctly)
{
    using namespace ::testing;
    MirEvent expected;
    expected.type = mir_event_type_motion;
    expected.motion.pointer_count = 1;
    expected.motion.event_time = std::chrono::nanoseconds(2);
    expected.motion.device_id = 3;
    expected.motion.source_id = 4;
    expected.motion.action = mir_motion_action_scroll;
    expected.motion.modifiers = 6;
    expected.motion.button_state =
        static_cast<MirMotionButton>(mir_motion_button_forward | mir_motion_button_secondary);

    auto & pointer = expected.motion.pointer_coordinates[0];
    pointer.id = 1;
    pointer.x = 12.0f;
    pointer.y = 13.0f;
    pointer.touch_major = 14.0f;
    pointer.touch_minor = 15.0f;
    pointer.size = 16.0f;
    pointer.pressure = 17.0f;
    pointer.orientation = 18.0f;
    pointer.vscroll = 19.0f;
    pointer.hscroll = 20.0f;
    pointer.tool_type = mir_motion_tool_type_finger;

    coords[0].setAxisValue(AMOTION_EVENT_AXIS_X, pointer.x);
    coords[0].setAxisValue(AMOTION_EVENT_AXIS_Y, pointer.y);
    coords[0].setAxisValue(AMOTION_EVENT_AXIS_TOUCH_MAJOR, pointer.touch_major);
    coords[0].setAxisValue(AMOTION_EVENT_AXIS_TOUCH_MINOR, pointer.touch_minor);
    coords[0].setAxisValue(AMOTION_EVENT_AXIS_SIZE, pointer.size);
    coords[0].setAxisValue(AMOTION_EVENT_AXIS_PRESSURE, pointer.pressure);
    coords[0].setAxisValue(AMOTION_EVENT_AXIS_ORIENTATION, pointer.orientation);
    coords[0].setAxisValue(AMOTION_EVENT_AXIS_VSCROLL, pointer.vscroll);
    coords[0].setAxisValue(AMOTION_EVENT_AXIS_HSCROLL, pointer.hscroll);
    properties[0].id = pointer.id;
    properties[0].toolType = pointer.tool_type;
    InSequence seq;
    EXPECT_CALL(dispatcher, dispatch(mt::MirTouchEventMatches(expected))).Times(1);

    droidinput::NotifyMotionArgs notified(std::chrono::nanoseconds(expected.motion.event_time),
                                          expected.motion.device_id,
                                          expected.motion.source_id,
                                          default_policy_flags,
                                          expected.motion.action,
                                          0, /* flags */
                                          expected.motion.modifiers,
                                          expected.motion.button_state,
                                          0, /* edge flags */
                                          expected.motion.pointer_count,
                                          properties,
                                          coords,
                                          0, 0, /* unused x/y precision */
                                          expected.motion.event_time);

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

    droidinput::NotifyKeyArgs tester(some_time, device_id, source_id,
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


