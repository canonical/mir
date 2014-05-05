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

#include "src/server/input/android/input_translator.cpp"
#include "mir_test_doubles/mock_input_dispatcher.h"
#include "mir_test/fake_shared.h"
#include "mir_test/client_event_matchers.h"

#include "InputListener.h"

namespace droidinput = android;
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
    }
    const nsecs_t some_time = 13;
    const nsecs_t later_time = 14;
    const int32_t device_id = 13;
    const uint32_t source_id = 13;
    const uint32_t default_flags = 0;
    const uint32_t default_policy_flags = 0;
    const int32_t no_flags= 0;
    const int32_t no_modifiers = 0;
    int32_t arbitrary_key_code = 17;
    int32_t arbitrary_scan_code = 17;
};

struct PolicyFlagTestParameter
{
    uint32_t policy_flag;
    MirKeyFlag expected_key_flag;
    unsigned int expected_modifiers;
};

class InputTranslatorWithPolicyParam : public InputTranslator,
    public ::testing::WithParamInterface<PolicyFlagTestParameter>
{
};

}

TEST_F(InputTranslator, notifies_configuration_change)
{
    using namespace ::testing;

    EXPECT_CALL(dispatcher, configuration_changed(some_time)).Times(1);

    droidinput::NotifyConfigurationChangedArgs change(some_time);
    translator.notifyConfigurationChanged(&change);
}

TEST_F(InputTranslator, notifies_device_reset)
{
    using namespace ::testing;

    EXPECT_CALL(dispatcher, device_reset(device_id,later_time)).Times(1);

    droidinput::NotifyDeviceResetArgs reset(later_time, device_id);
    translator.notifyDeviceReset(&reset);
}

TEST_F(InputTranslator, ignores_invalid_key_events)
{
    using namespace ::testing;

    EXPECT_CALL(dispatcher, dispatch(_)).Times(0);

    translator.notifyKey(nullptr);

    int32_t invalid_action = 5;
    droidinput::NotifyKeyArgs key(some_time, device_id, source_id, default_policy_flags, invalid_action, no_flags, arbitrary_key_code, arbitrary_scan_code, no_modifiers, later_time);

    translator.notifyKey(&key);
}

TEST_F(InputTranslator, forwards_and_convertes_up_down_key_notifications)
{
    using namespace ::testing;

    InSequence seq;
    EXPECT_CALL(dispatcher, dispatch(mt::KeyDownEvent())).Times(1);
    EXPECT_CALL(dispatcher, dispatch(mt::KeyUpEvent())).Times(1);

    droidinput::NotifyKeyArgs down(some_time, device_id, source_id, default_policy_flags, mir_key_action_down,
                                   no_flags, arbitrary_key_code, arbitrary_scan_code, no_modifiers, later_time);
    droidinput::NotifyKeyArgs up(some_time, device_id, source_id, default_policy_flags, mir_key_action_up,
                                 no_flags, arbitrary_key_code, arbitrary_scan_code, no_modifiers, later_time);

    translator.notifyKey(&down);
    translator.notifyKey(&up);
}

TEST_P(InputTranslatorWithPolicyParam, forwards_policy_modifiers_as_flags_and_modifiers)
{
    using namespace ::testing;

    EXPECT_CALL(dispatcher,
                dispatch(
                    AllOf(
                        mt::KeyWithFlag(GetParam().expected_key_flag),
                        mt::KeyWithModifiers(GetParam().expected_modifiers)
                        )
                    )
                ).Times(1);

    droidinput::NotifyKeyArgs tester(some_time, device_id, source_id,
                                     default_policy_flags | GetParam().policy_flag, mir_key_action_down,
                                     no_flags, arbitrary_key_code, arbitrary_scan_code, no_modifiers, later_time);

    translator.notifyKey(&tester);
}

const MirKeyFlag default_key_flag = static_cast<MirKeyFlag>(0);
INSTANTIATE_TEST_CASE_P(VariousPolicyFlags, InputTranslatorWithPolicyParam,
                        ::testing::Values(PolicyFlagTestParameter{droidinput::POLICY_FLAG_CAPS_LOCK, default_key_flag,
                                                                  mir_key_modifier_caps_lock},
                                          PolicyFlagTestParameter{droidinput::POLICY_FLAG_ALT, default_key_flag,
                                                                  mir_key_modifier_alt | mir_key_modifier_alt_left},
                                          PolicyFlagTestParameter{droidinput::POLICY_FLAG_ALT_GR, default_key_flag,
                                                                  mir_key_modifier_alt | mir_key_modifier_alt_right},
                                          PolicyFlagTestParameter{droidinput::POLICY_FLAG_FUNCTION, default_key_flag,
                                                                  mir_key_modifier_function},
                                          PolicyFlagTestParameter{droidinput::POLICY_FLAG_SHIFT, default_key_flag,
                                                                  mir_key_modifier_shift | mir_key_modifier_shift_left},
                                          PolicyFlagTestParameter{droidinput::POLICY_FLAG_VIRTUAL,
                                                                  mir_key_flag_virtual_hard_key,
                                                                  mir_key_modifier_none}));


