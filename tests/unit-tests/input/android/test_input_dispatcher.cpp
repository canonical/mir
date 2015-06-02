/*
 * Copyright © 2015 Canonical Ltd.
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

#include "InputDispatcher.h"
#include "InputApplication.h"
#include "InputWindow.h"
#include "InputWindow.h"

#include "mir/input/android/android_input_lexicon.h"
#include "mir/events/event_private.h"
#include "mir/report/legacy_input_report.h"
#include "src/server/report/null_report_factory.h"
#include "src/server/input/android/event_filter_dispatcher_policy.h"

#include "mir_test_doubles/stub_input_enumerator.h"
#include "mir_test_doubles/mock_event_filter.h"
#include "mir_test_doubles/null_logger.h"
#include "mir_test/fake_shared.h"
#include "mir_test/event_matchers.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <linux/input.h>

namespace mt = mir::test;
namespace mtd = mt::doubles;
namespace mia = mir::input::android;
using namespace std::literals;

namespace
{
struct MockInputDispatcherPolicy : mia::EventFilterDispatcherPolicy
{
    using mia::EventFilterDispatcherPolicy::EventFilterDispatcherPolicy;
    MOCK_METHOD1(handle,void(MirEvent const& event));
    std::chrono::nanoseconds interceptKeyBeforeDispatching(
        const android::sp<android::InputWindowHandle>&,
            const android::KeyEvent* keyEvent, uint32_t)
    {
        auto mir_ev = mia::Lexicon::translate(keyEvent);
        handle(*mir_ev);
        return 0ns;
    }
};


struct InputDispatcher : testing::Test
{
    mtd::StubInputEnumerator enumerator;
    testing::NiceMock<mtd::MockEventFilter> filter;
    testing::NiceMock<MockInputDispatcherPolicy> filter_policy{mt::fake_shared(filter), true};
    android::InputDispatcher dispatcher{mt::fake_shared(filter_policy), mir::report::null_input_report(), mt::fake_shared(enumerator)};
    mtd::NullLogger logger;
    InputDispatcher()
    {
        mir::report::legacy_input::initialize(mt::fake_shared(logger));
    }

    android::NotifyKeyArgs create_key_event(int32_t key_code, int32_t scan_code, int32_t action = AKEY_EVENT_ACTION_DOWN)
    {
        static auto time = 0ns;
        time += 1ns;
        int32_t deviceid = 0;
        uint32_t source = AINPUT_SOURCE_KEYBOARD;
        uint32_t policy_flag = 0;
        int32_t flag = 0;
        int32_t meta_state = 0;
        auto down_time = 0ns;

        return android::NotifyKeyArgs{time, deviceid, source, policy_flag, action, flag, key_code, scan_code, meta_state, down_time};
    }
};

}

TEST_F(InputDispatcher, forwards_multiple_downs)
{
    using namespace ::testing;
    InSequence seq;
    EXPECT_CALL(filter_policy, handle(AllOf(mt::KeyDownEvent(), mt::KeyOfScanCode(KEY_RIGHTSHIFT))));
    EXPECT_CALL(filter_policy, handle(AllOf(mt::KeyDownEvent(), mt::KeyOfScanCode(KEY_M))));
    auto first_key_event = create_key_event(0, KEY_RIGHTSHIFT);
    auto second_key_event = create_key_event(0, KEY_M);

    dispatcher.notifyKey(&first_key_event);
    dispatcher.notifyKey(&second_key_event);

    dispatcher.dispatchOnce();
    dispatcher.dispatchOnce();
    dispatcher.dispatchOnce();
}

TEST_F(InputDispatcher, detects_multiple_down_of_same_key)
{
    using namespace ::testing;
    InSequence seq;
    EXPECT_CALL(filter_policy, handle(AllOf(mt::KeyDownEvent(), mt::KeyOfScanCode(KEY_M))));
    EXPECT_CALL(filter_policy, handle(AllOf(mt::KeyRepeatEvent(), mt::KeyOfScanCode(KEY_M))));
    auto first_key_event = create_key_event(0, KEY_M);
    auto second_key_event = create_key_event(0, KEY_M);

    dispatcher.notifyKey(&first_key_event);
    dispatcher.notifyKey(&second_key_event);

    dispatcher.dispatchOnce();
    dispatcher.dispatchOnce();
    dispatcher.dispatchOnce();
}
