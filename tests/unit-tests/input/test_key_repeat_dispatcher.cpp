/*
 * Copyright © 2015 Canonical Ltd.
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
 *
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#include "src/server/input/key_repeat_dispatcher.h"

#include "mir/events/event_private.h"
#include "mir/events/event_builders.h"
#include "mir/time/alarm.h"
#include "mir/time/alarm_factory.h"
#include "mir/cookie/authority.h"
#include "mir/input/input_device_observer.h"
#include "mir/input/mir_pointer_config.h"
#include "mir/input/mir_touchpad_config.h"
#include "mir/input/mir_touchscreen_config.h"
#include "mir/input/mir_keyboard_config.h"
#include "mir/input/device.h"

#include "mir/test/fake_shared.h"
#include "mir/test/event_matchers.h"
#include "mir/test/doubles/mock_input_dispatcher.h"
#include "mir/test/doubles/mock_input_device_hub.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mi = mir::input;
namespace mev = mir::events;
namespace mt = mir::test;
namespace mtd = mt::doubles;

using namespace ::testing;

namespace
{
struct MockAlarm : public mir::time::Alarm
{
    MOCK_METHOD0(cancel, bool());
    MOCK_CONST_METHOD0(state, mir::time::Alarm::State());
    MOCK_METHOD1(reschedule_in, bool(std::chrono::milliseconds));
    MOCK_METHOD1(reschedule_for, bool(mir::time::Timestamp));

    // destructor cancels the alarm
    ~MockAlarm()
    {
        cancel();
    }
};

struct MockAlarmFactory : public mir::time::AlarmFactory
{
    MOCK_METHOD1(create_alarm_adapter, mir::time::Alarm*(std::function<void()> const&));
    std::unique_ptr<mir::time::Alarm> create_alarm(std::function<void()> const& cb)
    {
        return std::unique_ptr<mir::time::Alarm>(create_alarm_adapter(cb));
    }

    std::unique_ptr<mir::time::Alarm> create_alarm(std::unique_ptr<mir::LockableCallback>)
    {
        return nullptr;
    }
};

struct StubDevice : public mi::Device
{
    MirInputDeviceId device_id;
    std::string device_name;

    StubDevice(MirInputDeviceId id) : device_id(id) {}
    StubDevice(MirInputDeviceId id, std::string device_name) : device_id{id}, device_name{device_name}
    {
    }
    MirInputDeviceId id() const
    {
        return device_id;
    }
    mi::DeviceCapabilities capabilities() const {return mi::DeviceCapability::keyboard;}
    std::string name() const {return device_name;}
    std::string unique_id() const {return {};}

    mir::optional_value<MirPointerConfig> pointer_configuration() const {return {};}
    void apply_pointer_configuration(MirPointerConfig const&) {;}
    mir::optional_value<MirTouchpadConfig> touchpad_configuration() const {return {};}
    void apply_touchpad_configuration(MirTouchpadConfig const&) {}
    mir::optional_value<MirKeyboardConfig> keyboard_configuration() const {return {};}
    void apply_keyboard_configuration(MirKeyboardConfig const&) {}
    mir::optional_value<MirTouchscreenConfig> touchscreen_configuration() const {return {};}
    void apply_touchscreen_configuration(MirTouchscreenConfig const&) {}
};

struct KeyRepeatDispatcher : public testing::Test
{
    KeyRepeatDispatcher(bool on_arale = false)
        : dispatcher(mock_next_dispatcher, mock_alarm_factory, cookie_authority, true, repeat_time, repeat_delay, on_arale)
    {
        ON_CALL(hub,add_observer(_)).WillByDefault(SaveArg<0>(&observer));
        dispatcher.set_input_device_hub(mt::fake_shared(hub));
    }
    void simulate_device_removal()
    {
        StubDevice dev(test_device);
        observer->device_removed(mt::fake_shared(dev));
        observer->changes_complete();
    }

    const MirInputDeviceId test_device = 123;
    std::shared_ptr<mtd::MockInputDispatcher> mock_next_dispatcher = std::make_shared<mtd::MockInputDispatcher>();
    std::shared_ptr<MockAlarmFactory> mock_alarm_factory = std::make_shared<MockAlarmFactory>();
    std::shared_ptr<mir::cookie::Authority> cookie_authority = mir::cookie::Authority::create();
    std::chrono::milliseconds const repeat_time{2};
    std::chrono::milliseconds const repeat_delay{1};
    std::shared_ptr<mi::InputDeviceObserver> observer;
    NiceMock<mtd::MockInputDeviceHub> hub;
    mi::KeyRepeatDispatcher dispatcher;

    mir::EventUPtr a_key_down_event()
    {
        return mev::make_event(test_device, std::chrono::nanoseconds(0), std::vector<uint8_t>{}, mir_keyboard_action_down, 0, 0, mir_input_event_modifier_alt);
    }

    mir::EventUPtr a_key_up_event()
    {
        return mev::make_event(test_device, std::chrono::nanoseconds(0), std::vector<uint8_t>{}, mir_keyboard_action_up, 0, 0, mir_input_event_modifier_alt);
    }
};

struct KeyRepeatDispatcherOnArale : KeyRepeatDispatcher
{
    KeyRepeatDispatcherOnArale() : KeyRepeatDispatcher(true){};
    MirInputDeviceId mtk_id = 32;
    void add_mtk_tpd()
    {
        StubDevice dev(mtk_id, "mtk-tpd");
        observer->device_added(mt::fake_shared(dev));
        observer->changes_complete();
    }

    mir::EventUPtr mtk_key_down_event()
    {
        auto const home_button = 53;
        return mev::make_event(mtk_id, std::chrono::nanoseconds(0), std::vector<uint8_t>{}, mir_keyboard_action_down, 0,
                               home_button, mir_input_event_modifier_none);
    }
};
}

TEST_F(KeyRepeatDispatcher, forwards_start_stop)
{
    InSequence seq;
    EXPECT_CALL(*mock_next_dispatcher, start()).Times(1);
    EXPECT_CALL(*mock_next_dispatcher, stop()).Times(1);

    dispatcher.start();
    dispatcher.stop();
}

TEST_F(KeyRepeatDispatcher, schedules_alarm_to_repeat_key_down)
{
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
    dispatcher.dispatch(a_key_down_event());
    // Trigger the repeat
    alarm_function();
    // Trigger the cancel
    dispatcher.dispatch(a_key_up_event());
}

TEST_F(KeyRepeatDispatcher, stops_repeat_on_device_removal)
{
    MockAlarm *mock_alarm = new MockAlarm;
    std::function<void()> alarm_function;
    bool alarm_canceled = false;

    InSequence seq;
    EXPECT_CALL(*mock_alarm_factory, create_alarm_adapter(_)).Times(1).
        WillOnce(DoAll(SaveArg<0>(&alarm_function), Return(mock_alarm)));
    // Once for initial down and again when invoked
    EXPECT_CALL(*mock_alarm, reschedule_in(repeat_time)).Times(1).WillOnce(Return(true));
    EXPECT_CALL(*mock_next_dispatcher, dispatch(mt::KeyDownEvent())).Times(1);
    EXPECT_CALL(*mock_next_dispatcher, dispatch(mt::KeyRepeatEvent())).Times(1);
    EXPECT_CALL(*mock_alarm, reschedule_in(repeat_delay)).Times(1).WillOnce(Return(true));
    ON_CALL(*mock_alarm, cancel()).WillByDefault(Invoke([&](){alarm_canceled = true; return true;}));

    dispatcher.dispatch(a_key_down_event());

    alarm_function();
    Mock::VerifyAndClearExpectations(mock_alarm); // mock_alarm will be deleted after this

    simulate_device_removal();
    EXPECT_THAT(alarm_canceled, Eq(true));
}

TEST_F(KeyRepeatDispatcherOnArale, no_repeat_alarm_on_mtk_tpd)
{
    EXPECT_CALL(*mock_alarm_factory, create_alarm_adapter(_)).Times(0);
    EXPECT_CALL(*mock_next_dispatcher, dispatch(mt::KeyDownEvent())).Times(1);
    EXPECT_CALL(*mock_next_dispatcher, dispatch(mt::KeyRepeatEvent())).Times(0);

    add_mtk_tpd();
    dispatcher.dispatch(mtk_key_down_event());
}

TEST_F(KeyRepeatDispatcherOnArale, repeat_for_regular_keys)
{
    MockAlarm *mock_alarm = new MockAlarm;
    std::function<void()> alarm_function;

    EXPECT_CALL(*mock_alarm_factory, create_alarm_adapter(_)).Times(1).
        WillOnce(DoAll(SaveArg<0>(&alarm_function), Return(mock_alarm)));
    // Once for initial down and again when invoked
    EXPECT_CALL(*mock_alarm, reschedule_in(repeat_delay)).Times(1).WillOnce(Return(true));
    EXPECT_CALL(*mock_alarm, reschedule_in(repeat_time)).Times(1).WillOnce(Return(true));
    EXPECT_CALL(*mock_next_dispatcher, dispatch(mt::KeyDownEvent())).Times(1);
    EXPECT_CALL(*mock_next_dispatcher, dispatch(mt::KeyRepeatEvent())).Times(1);

    add_mtk_tpd();
    dispatcher.dispatch(a_key_down_event());
    alarm_function();
}
