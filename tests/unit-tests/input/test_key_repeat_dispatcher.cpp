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

#include "src/server/input/key_repeat_dispatcher.h"

#include "mir/events/event_private.h"
#include "mir/events/event_builders.h"
#include "mir/time/alarm.h"
#include "mir/time/alarm_factory.h"

#include "mir/test/event_matchers.h"
#include "mir/test/doubles/mock_input_dispatcher.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mi = mir::input;
namespace mev = mir::events;
namespace mt = mir::test;
namespace mtd = mt::doubles;

namespace
{
struct MockAlarm : public mir::time::Alarm
{
    MOCK_METHOD0(cancel, bool());
    MOCK_CONST_METHOD0(state, mir::time::Alarm::State());
    MOCK_METHOD1(reschedule_in, bool(std::chrono::milliseconds));
    MOCK_METHOD1(reschedule_for, bool(mir::time::Timestamp));
};

struct MockAlarmFactory : public mir::time::AlarmFactory
{
    MOCK_METHOD1(create_alarm_adapter, mir::time::Alarm*(std::function<void()> const&));
    std::unique_ptr<mir::time::Alarm> create_alarm(std::function<void()> const& cb)
    {
        return std::unique_ptr<mir::time::Alarm>(create_alarm_adapter(cb));
    }

    std::unique_ptr<mir::time::Alarm> create_alarm(std::shared_ptr<mir::LockableCallback> const&)
    {
        return nullptr;
    }
};

struct KeyRepeatDispatcher : public testing::Test
{
    KeyRepeatDispatcher()
        : dispatcher(mock_next_dispatcher, mock_alarm_factory, true, repeat_time, repeat_delay)
    {
    }
    std::shared_ptr<mtd::MockInputDispatcher> mock_next_dispatcher = std::make_shared<mtd::MockInputDispatcher>();
    std::shared_ptr<MockAlarmFactory> mock_alarm_factory = std::make_shared<MockAlarmFactory>();
    std::chrono::milliseconds const repeat_time{2};
    std::chrono::milliseconds const repeat_delay{1};
    mi::KeyRepeatDispatcher dispatcher;
};
}

TEST_F(KeyRepeatDispatcher, forwards_start_stop)
{
    using namespace ::testing;

    InSequence seq;
    EXPECT_CALL(*mock_next_dispatcher, start()).Times(1);
    EXPECT_CALL(*mock_next_dispatcher, stop()).Times(1);

    dispatcher.start();
    dispatcher.stop();
}

namespace
{
mir::EventUPtr a_key_down_event()
{
    return mev::make_event(0, std::chrono::nanoseconds(0), mir_keyboard_action_down, 0, 0, mir_input_event_modifier_alt);
}
mir::EventUPtr a_key_up_event()
{
    return mev::make_event(0, std::chrono::nanoseconds(0), mir_keyboard_action_up, 0, 0, mir_input_event_modifier_alt);
}
}

TEST_F(KeyRepeatDispatcher, schedules_alarm_to_repeat_key_down)
{
    using namespace ::testing;
    
    MockAlarm *mock_alarm = new MockAlarm; // deleted by AlarmFactory
    std::function<void()> alarm_function;

    InSequence seq;
    EXPECT_CALL(*mock_alarm_factory, create_alarm_adapter(_)).Times(1).
        WillOnce(DoAll(SaveArg<0>(&alarm_function), Return(mock_alarm)));
    // Once for initial down and again when invoked
    EXPECT_CALL(*mock_alarm, reschedule_in(repeat_time)).Times(1).WillOnce(Return(true));
    EXPECT_CALL(*mock_next_dispatcher, dispatch(mt::KeyDownEvent())).Times(1);
    EXPECT_CALL(*mock_next_dispatcher, dispatch(mt::KeyRepeatEvent())).Times(1);
    EXPECT_CALL(*mock_alarm, reschedule_in(repeat_delay)).Times(1).WillOnce(Return(true));
    EXPECT_CALL(*mock_next_dispatcher, dispatch(mt::KeyUpEvent())).Times(1);

    // Schedule the repeat
    dispatcher.dispatch(*a_key_down_event());
    // Trigger the repeat
    alarm_function();
    // Trigger the cancel
    dispatcher.dispatch(*a_key_up_event());
}
