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

#include "mir/test/doubles/mock_scene_session.h"
#include "mir/test/doubles/fake_alarm_factory.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mt = mir::time;
namespace ms = mir::scene;
namespace mtd = mir::test::doubles;

namespace
{
class MockObserver : public ms::ApplicationNotRespondingDetector::Observer
{
public:
    MOCK_METHOD1(session_unresponsive, void(ms::Session const*));
    MOCK_METHOD1(session_now_responsive, void(ms::Session const*));
};
}

TEST(TimeoutApplicationNotRespondingDetector, pings_registered_sessions_on_schedule)
{
    using namespace testing;
    using namespace std::literals::chrono_literals;
    
    mtd::FakeAlarmFactory fake_alarms;
    
    ms::TimeoutApplicationNotRespondingDetector detector{fake_alarms, 1s};
    
    bool first_session_pinged{false}, second_session_pinged{false};
    
    NiceMock<mtd::MockSceneSession> session_one, session_two;
    
    detector.register_session(&session_one, [&first_session_pinged]() { first_session_pinged = true; });
    detector.register_session(&session_two, [&second_session_pinged]() { second_session_pinged = true; });
    
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

    mtd::FakeAlarmFactory fake_alarms;

    ms::TimeoutApplicationNotRespondingDetector detector{fake_alarms, 1s};

    int first_session_pinged{0}, second_session_pinged{0};

    NiceMock<mtd::MockSceneSession> session_one, session_two;

    detector.register_session(&session_one, [&first_session_pinged]() { first_session_pinged++; });
    detector.register_session(&session_two, [&second_session_pinged]() { second_session_pinged++; });

    int const expected_count{900};

    for (int i = 0; i < expected_count ; ++i)
    {
        fake_alarms.advance_by(1001ms);
        detector.pong_received(&session_one);
        detector.pong_received(&session_two);
    }

    EXPECT_THAT(first_session_pinged, Eq(expected_count));
    EXPECT_THAT(second_session_pinged, Eq(expected_count));
}

TEST(TimeoutApplicationNotRespondingDetector, triggers_anr_signal_when_session_fails_to_pong)
{
    using namespace testing;
    using namespace std::literals::chrono_literals;

    mtd::FakeAlarmFactory fake_alarms;

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

    detector.register_session(&session_one, [](){});

    fake_alarms.advance_by(1001ms);
    // Should now have pung, but not marked as unresponsive
    EXPECT_FALSE(session_not_responding);

    // Now a full ping cycle has elapsed, should have marked as unresponsive
    fake_alarms.advance_by(1001ms);
    EXPECT_TRUE(session_not_responding);
}

TEST(TimeoutApplicationNotRespondingDetector, does_not_trigger_anr_when_session_pongs)
{
    using namespace testing;
    using namespace std::literals::chrono_literals;

    mtd::FakeAlarmFactory fake_alarms;

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

    detector.register_session(&session_one, [](){});

    fake_alarms.advance_by(1001ms);
    // Should now have pung, but not marked as unresponsive
    EXPECT_FALSE(session_not_responding);

    detector.pong_received(&session_one);

    // Now a full ping cycle has elapsed, but the session has pung and so isn't unresponsive
    fake_alarms.advance_by(1001ms);
    EXPECT_FALSE(session_not_responding);
}

TEST(TimeoutApplicationNotRespondingDetector, triggers_now_responsive_signal_when_ponged)
{
    using namespace testing;
    using namespace std::literals::chrono_literals;

    mtd::FakeAlarmFactory fake_alarms;

    ms::TimeoutApplicationNotRespondingDetector detector{fake_alarms, 1s};

    bool session_not_responding{false};
    auto observer = std::make_shared<NiceMock<MockObserver>>();
    ON_CALL(*observer, session_unresponsive(_))
        .WillByDefault(Invoke([&session_not_responding](auto /*session*/)
    {
        session_not_responding = true;
    }));
    ON_CALL(*observer, session_now_responsive(_))
        .WillByDefault(Invoke([&session_not_responding](auto /*session*/)
    {
        session_not_responding = false;
    }));
    detector.register_observer(observer);

    NiceMock<mtd::MockSceneSession> session_one;

    detector.register_session(&session_one, [](){});

    fake_alarms.advance_by(1001ms);
    // Should now have pung, but not marked as unresponsive
    EXPECT_FALSE(session_not_responding);

    // Now a full ping cycle has elapsed, should have marked as unresponsive
    fake_alarms.advance_by(1001ms);
    EXPECT_TRUE(session_not_responding);

    detector.pong_received(&session_one);

    EXPECT_FALSE(session_not_responding);
}

TEST(TimeoutApplicationNotRespondingDetector, fiddling_with_sessions_from_callbacks_doesnt_deadlock)
{
    using namespace testing;
    using namespace std::literals::chrono_literals;

    mtd::FakeAlarmFactory fake_alarms;

    ms::TimeoutApplicationNotRespondingDetector detector{fake_alarms, 1s};

    NiceMock<mtd::MockSceneSession> session_one, session_two, session_three;

    auto observer = std::make_shared<NiceMock<MockObserver>>();
    ON_CALL(*observer, session_unresponsive(_))
        .WillByDefault(Invoke([&detector, &session_two](auto /*session*/)
    {
        detector.register_session(&session_two, [](){});
    }));
    ON_CALL(*observer, session_now_responsive(_))
        .WillByDefault(Invoke([&detector, &session_three](auto /*session*/)
    {
        detector.unregister_session(&session_three);
    }));
    detector.register_observer(observer);

    detector.register_session(&session_one, [](){});
    detector.register_session(&session_three, [](){});

    fake_alarms.advance_by(1001ms);
    // Should now have pung

    fake_alarms.advance_by(1001ms);
    // Should now have marked session three unresponsive, registering session two

    detector.pong_received(&session_three);
    // And now session three sholud be morked as responsive, unregistering itself
}

TEST(TimeoutApplicationNotRespondingDetector, does_not_generate_signals_after_unregister)
{
    using namespace testing;
    using namespace std::literals::chrono_literals;

    mtd::FakeAlarmFactory fake_alarms;

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

    detector.register_session(&session_one, [](){});

    fake_alarms.advance_by(1001ms);
    // Should now have pung, but not marked as unresponsive
    EXPECT_FALSE(session_not_responding);

    // Now a full ping cycle has elapsed, should have marked as unresponsive
    fake_alarms.advance_by(1001ms);
    EXPECT_TRUE(session_not_responding);

    // Mark session as responding
    detector.pong_received(&session_one);
    session_not_responding = false;

    detector.unregister_observer(observer);

    fake_alarms.advance_by(1001ms);
    fake_alarms.advance_by(1001ms);

    // Notification should not have been generated
    EXPECT_FALSE(session_not_responding);
}

TEST(TimeoutApplicationNotRespondingDetector, does_not_schedule_alarm_when_no_sessions)
{
    using namespace testing;
    using namespace std::literals::chrono_literals;

    mtd::FakeAlarmFactory fake_alarms;

    ms::TimeoutApplicationNotRespondingDetector detector{fake_alarms, 1s};

    // Go through several ping cycles.
    fake_alarms.advance_smoothly_by(5000ms);

    EXPECT_THAT(fake_alarms.wakeup_count(), Eq(0));

    NiceMock<mtd::MockSceneSession> session;

    detector.register_session(&session, [](){});

    fake_alarms.advance_smoothly_by(5000ms);
    EXPECT_THAT(fake_alarms.wakeup_count(), Gt(0));

    int const previous_wakeup_count{fake_alarms.wakeup_count()};

    detector.unregister_session(&session);

    fake_alarms.advance_smoothly_by(5000ms);

    // We allow one extra wakeup to notice there are no active sessions.
    EXPECT_THAT(fake_alarms.wakeup_count(), Le(previous_wakeup_count + 1));
}

TEST(TimeoutApplicationNotRespondingDetector, does_not_schedule_alarm_when_all_sessions_are_unresponsive)
{
    using namespace testing;
    using namespace std::literals::chrono_literals;

    mtd::FakeAlarmFactory fake_alarms;

    ms::TimeoutApplicationNotRespondingDetector detector{fake_alarms, 1s};

    NiceMock<mtd::MockSceneSession> session;
    bool session_unresponsive{false};

    auto observer = std::make_shared<NiceMock<MockObserver>>();
    ON_CALL(*observer, session_unresponsive(_))
        .WillByDefault(Invoke([&session_unresponsive](auto /*session*/)
    {
        session_unresponsive = true;
    }));
    detector.register_observer(observer);

    detector.register_session(&session, [](){});

    fake_alarms.advance_smoothly_by(5000ms);

    ASSERT_TRUE(session_unresponsive);
    EXPECT_THAT(fake_alarms.wakeup_count(), Gt(0));

    int const previous_wakeup_count{fake_alarms.wakeup_count()};

    // All the sessions are now unresponsive, so we shouldn't be triggering wakeups
    fake_alarms.advance_smoothly_by(5000ms);
    EXPECT_THAT(fake_alarms.wakeup_count(), Eq(previous_wakeup_count));
}

TEST(TimeoutApplicationNotRespondingDetector, session_switches_between_responsive_and_unresponsive)
{
    using namespace testing;
    using namespace std::literals::chrono_literals;

    mtd::FakeAlarmFactory fake_alarms;

    ms::TimeoutApplicationNotRespondingDetector detector{fake_alarms, 1s};

    NiceMock<mtd::MockSceneSession> session;
    bool session_unresponsive{false};

    auto observer = std::make_shared<NiceMock<MockObserver>>();
    ON_CALL(*observer, session_unresponsive(_))
        .WillByDefault(Invoke([&session_unresponsive](auto /*session*/)
    {
        session_unresponsive = true;
    }));
    ON_CALL(*observer, session_now_responsive(_))
        .WillByDefault(Invoke([&session_unresponsive](auto /*session*/)
    {
        session_unresponsive = false;
    }));
    detector.register_observer(observer);

    detector.register_session(&session, [](){});

    fake_alarms.advance_smoothly_by(5000ms);

    EXPECT_TRUE(session_unresponsive);

    detector.pong_received(&session);
    EXPECT_FALSE(session_unresponsive);

    fake_alarms.advance_smoothly_by(5000ms);
    EXPECT_TRUE(session_unresponsive);

    detector.pong_received(&session);
    EXPECT_FALSE(session_unresponsive);
}

TEST(TimeoutApplicationNotRespondingDetector, sends_unresponsive_notification_only_once)
{
    using namespace testing;
    using namespace std::literals::chrono_literals;

    mtd::FakeAlarmFactory fake_alarms;

    ms::TimeoutApplicationNotRespondingDetector detector{fake_alarms, 1s};

    NiceMock<mtd::MockSceneSession> session_one;
    NiceMock<mtd::MockSceneSession> session_two;
    auto delayed_dispatch_pong = fake_alarms.create_alarm(
        [&detector, &session_two]()
        {
            detector.pong_received(&session_two);
        });
    int unresponsive_notifications{0};

    auto observer = std::make_shared<NiceMock<MockObserver>>();
    ON_CALL(*observer, session_unresponsive(_))
        .WillByDefault(Invoke([&unresponsive_notifications](auto /*session*/)
    {
        ++unresponsive_notifications;
    }));
    detector.register_observer(observer);

    detector.register_session(&session_one, [](){});
    // We need a responsive session to ensure the ANRDetector keeps rescheduling wakeups.
    //
    // We need the delayed_dispatch_pong so that we can do the pong outside of the
    // ping callback; otherwise this would deadlock.
    detector.register_session(&session_two,
        [&delayed_dispatch_pong]()
        {
            delayed_dispatch_pong->reschedule_in(1ms);
        });

    fake_alarms.advance_smoothly_by(5000ms);

    EXPECT_THAT(unresponsive_notifications, Eq(1));
}

TEST(TimeoutApplicationNotRespondingDetector, sends_pings_on_schedule_as_sessions_are_connecting)
{
    using namespace testing;
    using namespace std::literals::chrono_literals;

    mtd::FakeAlarmFactory fake_alarms;

    auto const cycle_time = 1s;
    ms::TimeoutApplicationNotRespondingDetector detector{fake_alarms, cycle_time};

    NiceMock<mtd::MockSceneSession> session;
    std::atomic<int> ping_count{0};
    auto delayed_dispatch_pong = fake_alarms.create_alarm(
        [&detector, &session, &ping_count]()
        {
            detector.pong_received(&session);
            ++ping_count;
        });

    detector.register_session(&session,
        [&delayed_dispatch_pong]()
        {
            delayed_dispatch_pong->reschedule_in(1ms);
        });


    auto duration = 0ms;
    auto const step = 500ms;

    // <= ensures we go past the threshold for the cycle
    while (duration <= 5s)
    {
        NiceMock<mtd::MockSceneSession> dummy_session;
        detector.register_session(&dummy_session, [](){});

        fake_alarms.advance_smoothly_by(step);
        duration += step;

        detector.unregister_session(&dummy_session);
    }

    EXPECT_THAT(ping_count, Ge(duration / cycle_time));
}
