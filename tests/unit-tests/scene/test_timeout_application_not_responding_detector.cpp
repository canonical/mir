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
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include "src/server/scene/timeout_application_not_responding_detector.h"

#include "mir/time/alarm_factory.h"
#include "mir/time/clock.h"
#include "mir_test_doubles/advanceable_clock.h"
#include "mir_test_doubles/mock_scene_session.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mt = mir::time;
namespace ms = mir::scene;
namespace mtd = mir::test::doubles;

namespace
{
class FakeAlarm : public mt::Alarm
{
public:
    FakeAlarm(std::function<void()> const& callback,
        std::shared_ptr<mir::time::Clock> const& clock)
        : callback{callback},
          alarm_state{State::cancelled},
          triggers_at{mir::time::Timestamp::max()},
          clock{clock}
    {
    }
    
    void time_updated()
    {
        if (clock->now() > triggers_at)
        {
            triggers_at = mir::time::Timestamp::max();
            alarm_state = State::triggered;
            callback();
        }
    }
    
    bool cancel() override
    {
        if (alarm_state == State::pending)
        {
            triggers_at = mir::time::Timestamp::max();
            alarm_state = State::cancelled;
            return true;
        }
        return false;
    }
    
    State state() const override
    {
        return alarm_state;
    }
    
    bool reschedule_in(std::chrono::milliseconds delay) override
    {
        bool rescheduled = alarm_state == State::pending;
        alarm_state = State::pending;
        triggers_at = clock->now() + std::chrono::duration_cast<mt::Duration >(delay);
        return rescheduled;
    }
    
    bool reschedule_for(mir::time::Timestamp timeout) override
    {
        bool rescheduled = alarm_state == State::pending;
        if (timeout > clock->now())
        {
            alarm_state = State::pending;
            triggers_at = timeout;            
        }
        else
        {
            callback();
            triggers_at = mir::time::Timestamp::max();
            alarm_state = State::triggered;
        }
        return rescheduled;
    }
    
private:
    std::function<void()> const callback;
    State alarm_state;
    mir::time::Timestamp triggers_at;
    std::shared_ptr<mt::Clock> clock;
};

class FakeClockAlarmProvider : public mt::AlarmFactory
{
public:
    FakeClockAlarmProvider()
        : clock{std::make_shared<mtd::AdvanceableClock>()}
    {
    }
    
    std::unique_ptr<mt::Alarm> create_alarm(std::function<void()> const& callback) override
    {
        std::unique_ptr<mt::Alarm> alarm = std::make_unique<FakeAlarm>(callback, clock);
        alarms.push_back(static_cast<FakeAlarm*>(alarm.get()));
        return alarm;
    }
    
    std::unique_ptr<mt::Alarm> create_alarm(std::shared_ptr<mir::LockableCallback> const& /*callback*/) override
    {
        throw std::logic_error{"Lockable alarm creation not implemented for fake alarms"};
    }

    void advance_by(mt::Duration step)
    {
        clock->advance_by(step);
        for (auto& alarm : alarms)
        {
            alarm->time_updated();
        }
    }
    
private:
    std::vector<FakeAlarm*> alarms;
    std::shared_ptr<mtd::AdvanceableClock> const clock;
};

class MockObserver : public ms::ApplicationNotRespondingDetector::Observer
{
public:
    MOCK_METHOD1(session_unresponsive, void(ms::Session const*));
};
}

TEST(TimeoutApplicationNotRespondingDetector, pings_registered_sessions_on_schedule)
{
    using namespace testing;
    using namespace std::literals::chrono_literals;
    
    FakeClockAlarmProvider fake_alarms;
    
    ms::TimeoutApplicationNotRespondingDetector detector{fake_alarms, 1s};
    
    bool first_session_pinged{false}, second_session_pinged{false};
    
    NiceMock<mtd::MockSceneSession> session_one, session_two;
    
    detector.register_session(session_one, [&first_session_pinged]() { first_session_pinged = true; });
    detector.register_session(session_two, [&second_session_pinged]() { second_session_pinged = true; });
    
    fake_alarms.advance_by(999ms);

    EXPECT_FALSE(first_session_pinged);
    EXPECT_FALSE(second_session_pinged);

    fake_alarms.advance_by(2ms);

    EXPECT_TRUE(first_session_pinged);
    EXPECT_TRUE(second_session_pinged);
}

TEST(TimeoutApplicationNotRespondingDetector, pings_repeatedly)
{
    using namespace testing;
    using namespace std::literals::chrono_literals;

    FakeClockAlarmProvider fake_alarms;

    ms::TimeoutApplicationNotRespondingDetector detector{fake_alarms, 1s};

    int first_session_pinged{0}, second_session_pinged{0};

    NiceMock<mtd::MockSceneSession> session_one, session_two;

    detector.register_session(session_one, [&first_session_pinged]() { first_session_pinged++; });
    detector.register_session(session_two, [&second_session_pinged]() { second_session_pinged++; });

    int const expected_count{900};

    for (int i = 0; i < expected_count ; ++i)
    {
        fake_alarms.advance_by(1001ms);
    }

    EXPECT_THAT(first_session_pinged, Eq(expected_count));
    EXPECT_THAT(second_session_pinged, Eq(expected_count));
}

TEST(TimeoutApplicationNotRespondingDetector, triggers_anr_signal_when_session_fails_to_pong)
{
    using namespace testing;
    using namespace std::literals::chrono_literals;

    FakeClockAlarmProvider fake_alarms;

    ms::TimeoutApplicationNotRespondingDetector detector{fake_alarms, 1s};

    bool session_not_responding{false};
    auto observer = std::make_shared<NiceMock<MockObserver>>();
    ON_CALL(*observer, session_unresponsive(_))
        .WillByDefault(Invoke([&session_not_responding](auto /*session*/)
    {
        session_not_responding = true;
    }));
    detector.register_observer(observer);

    NiceMock<mtd::MockSceneSession> session_one;

    detector.register_session(session_one, [](){});

    fake_alarms.advance_by(1001ms);
    // Should now have pung, but not marked as unresponsive
    EXPECT_FALSE(session_not_responding);

    // Now a full ping cycle has elapsed, should have marked as unresponsive
    fake_alarms.advance_by(1001ms);
    EXPECT_TRUE(session_not_responding);
}
