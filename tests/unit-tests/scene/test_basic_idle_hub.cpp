/*
 * Copyright © 2022 Canonical Ltd.
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
 */

#include "basic_idle_hub.h"
#include "mir/test/doubles/advanceable_clock.h"
#include "mir/test/doubles/fake_alarm_factory.h"
#include "mir/test/doubles/explicit_executor.h"
#include "mir/test/fake_shared.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace testing;
using namespace std::chrono_literals;
namespace ms = mir::scene;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;

namespace
{
struct MockObserver: ms::IdleStateObserver
{
    MOCK_METHOD0(idle, void());
    MOCK_METHOD0(active, void());
};

struct BasicIdleHub: Test
{
    mtd::AdvanceableClock clock;
    mtd::FakeAlarmFactory alarm_factory{};
    ms::BasicIdleHub hub{mt::fake_shared(clock), alarm_factory};
    mtd::ExplicitExecutor executor;

    void advance_by(mir::time::Duration step)
    {
        clock.advance_by(step);
        alarm_factory.advance_smoothly_by(step);
    }
};
}

TEST_F(BasicIdleHub, observer_marked_active_initially)
{
    auto const observer = std::make_shared<StrictMock<MockObserver>>();
    EXPECT_CALL(*observer, active());
    hub.register_interest(observer, executor, 5s);
    executor.execute();
}

TEST_F(BasicIdleHub, observer_marked_idle_initially_after_wait)
{
    auto const observer = std::make_shared<StrictMock<MockObserver>>();
    advance_by(6s);
    EXPECT_CALL(*observer, idle());
    hub.register_interest(observer, executor, 5s);
    executor.execute();
}

TEST_F(BasicIdleHub, observer_marked_active_initially_after_wait_and_poke)
{
    auto const observer = std::make_shared<StrictMock<MockObserver>>();
    advance_by(6s);
    hub.poke();
    advance_by(4s);
    EXPECT_CALL(*observer, active());
    hub.register_interest(observer, executor, 5s);
    executor.execute();
}

TEST_F(BasicIdleHub, observer_marked_idle_after_wait)
{
    auto const observer = std::make_shared<StrictMock<MockObserver>>();
    EXPECT_CALL(*observer, active()).Times(AnyNumber());
    hub.register_interest(observer, executor, 5s);
    advance_by(4s);
    executor.execute();
    EXPECT_CALL(*observer, idle());
    advance_by(6s);
    executor.execute();
}

TEST_F(BasicIdleHub, observer_marked_active_after_poke)
{
    auto const observer = std::make_shared<NiceMock<MockObserver>>();
    hub.register_interest(observer, executor, 5s);
    advance_by(6s);
    executor.execute();
    EXPECT_CALL(*observer, active());
    hub.poke();
    executor.execute();
}

TEST_F(BasicIdleHub, observer_can_remove_itself_in_idle_notification)
{
    auto const observer = std::make_shared<StrictMock<MockObserver>>();
    EXPECT_CALL(*observer, active()).Times(AnyNumber());
    hub.register_interest(observer, executor, 5s);
    executor.execute();
    EXPECT_CALL(*observer, idle()).WillOnce(Invoke([&]()
        {
            hub.unregister_interest(*observer);
        }));
    advance_by(6s);
    executor.execute();
}

TEST_F(BasicIdleHub, second_observer_with_same_timeout_marked_idle_initially_after_wait)
{
    auto const observer1 = std::make_shared<NiceMock<MockObserver>>();
    auto const observer2 = std::make_shared<StrictMock<MockObserver>>();
    hub.register_interest(observer1, executor, 5s);
    advance_by(6s);
    executor.execute();
    EXPECT_CALL(*observer2, idle());
    hub.register_interest(observer2, executor, 5s);
    executor.execute();
}

TEST_F(BasicIdleHub, second_observer_with_same_timeout_marked_active_initially_after_wait_and_poke)
{
    auto const observer1 = std::make_shared<NiceMock<MockObserver>>();
    auto const observer2 = std::make_shared<StrictMock<MockObserver>>();
    hub.register_interest(observer1, executor, 5s);
    advance_by(6s);
    hub.poke();
    executor.execute();
    EXPECT_CALL(*observer2, active());
    hub.register_interest(observer2, executor, 5s);
    executor.execute();
}

TEST_F(BasicIdleHub, multiple_observers_with_same_timeout_marked_idle_after_wait)
{
    auto const observer1 = std::make_shared<StrictMock<MockObserver>>();
    auto const observer2 = std::make_shared<StrictMock<MockObserver>>();
    auto const observer3 = std::make_shared<StrictMock<MockObserver>>();
    EXPECT_CALL(*observer1, active()).Times(AnyNumber());
    EXPECT_CALL(*observer2, active()).Times(AnyNumber());
    EXPECT_CALL(*observer3, active()).Times(AnyNumber());
    hub.register_interest(observer1, executor, 5s);
    hub.register_interest(observer2, executor, 5s);
    hub.register_interest(observer3, executor, 5s);
    executor.execute();
    EXPECT_CALL(*observer1, idle());
    EXPECT_CALL(*observer2, idle());
    EXPECT_CALL(*observer3, idle());
    advance_by(6s);
    executor.execute();
}

TEST_F(BasicIdleHub, multiple_observers_with_different_timeouts_marked_idle_at_correct_time)
{
    auto const observer1 = std::make_shared<StrictMock<MockObserver>>();
    auto const observer2 = std::make_shared<StrictMock<MockObserver>>();
    auto const observer3 = std::make_shared<StrictMock<MockObserver>>();
    EXPECT_CALL(*observer1, active()).Times(AnyNumber());
    EXPECT_CALL(*observer2, active()).Times(AnyNumber());
    EXPECT_CALL(*observer3, active()).Times(AnyNumber());
    hub.register_interest(observer1, executor, 5s);
    hub.register_interest(observer2, executor, 10s);
    hub.register_interest(observer3, executor, 12s);
    executor.execute();
    EXPECT_CALL(*observer1, idle());
    advance_by(6s);
    executor.execute();
    EXPECT_CALL(*observer2, idle());
    advance_by(5s);
    executor.execute();
    EXPECT_CALL(*observer3, idle());
    advance_by(2s);
    executor.execute();
}

TEST_F(BasicIdleHub, multiple_observers_with_different_timeouts_marked_active_after_poke)
{
    auto const observer1 = std::make_shared<NiceMock<MockObserver>>();
    auto const observer2 = std::make_shared<NiceMock<MockObserver>>();
    auto const observer3 = std::make_shared<NiceMock<MockObserver>>();
    hub.register_interest(observer1, executor, 5s);
    hub.register_interest(observer2, executor, 10s);
    hub.register_interest(observer3, executor, 12s);
    advance_by(13s);
    executor.execute();
    EXPECT_CALL(*observer1, active());
    EXPECT_CALL(*observer2, active());
    EXPECT_CALL(*observer3, active());
    hub.poke();
    executor.execute();
}

TEST_F(BasicIdleHub, observer_marked_idle_after_observer_with_same_timeout_removed)
{
    auto const observer1 = std::make_shared<StrictMock<MockObserver>>();
    auto const observer2 = std::make_shared<StrictMock<MockObserver>>();
    EXPECT_CALL(*observer1, active()).Times(AnyNumber());
    EXPECT_CALL(*observer2, active()).Times(AnyNumber());
    hub.register_interest(observer1, executor, 5s);
    hub.register_interest(observer2, executor, 5s);
    hub.unregister_interest(*observer1);
    executor.execute();
    EXPECT_CALL(*observer2, idle());
    advance_by(6s);
    executor.execute();
}

TEST_F(BasicIdleHub, observer_marked_idle_after_observer_with_shorter_timeout_removed)
{
    auto const observer1 = std::make_shared<StrictMock<MockObserver>>();
    auto const observer2 = std::make_shared<StrictMock<MockObserver>>();
    EXPECT_CALL(*observer1, active()).Times(AnyNumber());
    EXPECT_CALL(*observer2, active()).Times(AnyNumber());
    hub.register_interest(observer1, executor, 5s);
    hub.register_interest(observer2, executor, 10s);
    hub.unregister_interest(*observer1);
    executor.execute();
    EXPECT_CALL(*observer2, idle());
    advance_by(11s);
    executor.execute();
}

TEST_F(BasicIdleHub, observer_marked_idle_when_added_after_observer_with_longer_timeout)
{
    auto const observer1 = std::make_shared<StrictMock<MockObserver>>();
    auto const observer2 = std::make_shared<StrictMock<MockObserver>>();
    EXPECT_CALL(*observer1, active()).Times(AnyNumber());
    EXPECT_CALL(*observer2, active()).Times(AnyNumber());
    hub.register_interest(observer1, executor, 10s);
    hub.register_interest(observer2, executor, 5s);
    hub.unregister_interest(*observer1);
    executor.execute();
    EXPECT_CALL(*observer2, idle());
    advance_by(6s);
    executor.execute();
}

TEST_F(BasicIdleHub, observer_marked_idle_after_shorter_timeout_removed_and_poke)
{
    auto const observer1 = std::make_shared<StrictMock<MockObserver>>();
    auto const observer2 = std::make_shared<StrictMock<MockObserver>>();
    EXPECT_CALL(*observer1, active()).Times(AnyNumber());
    EXPECT_CALL(*observer2, active()).Times(AnyNumber());
    hub.register_interest(observer1, executor, 5s);
    hub.register_interest(observer2, executor, 10s);
    executor.execute();
    EXPECT_CALL(*observer1, idle());
    advance_by(6s);
    executor.execute();
    hub.unregister_interest(*observer1);
    hub.poke();
    advance_by(9s);
    executor.execute();
    EXPECT_CALL(*observer2, idle());
    advance_by(2s);
    executor.execute();
}

TEST_F(BasicIdleHub, inhibit_idle_when_uninhibited)
{
    // TODO - write
//    TEST_F(BasicIdleHub, observer_marked_active_after_poke)
//    {
//        auto const observer = std::make_shared<NiceMock<MockObserver>>();
//        hub.register_interest(observer, executor, 5s);
//        advance_by(6s);
//        executor.execute();
//        EXPECT_CALL(*observer, active());
//        hub.poke();
//        executor.execute();
//    }

//    auto const observer = std::make_shared<StrictMock<MockObserver>>();
//    hub.register_interest(observer, executor, 5s);
//    advance_by(6s);
//    executor.execute();
//    hub.poke();
////    hub.inhibit_idle();
//    executor.execute();
//    EXPECT_CALL(*observer, idle());
//    advance_by(6s);
//    executor.execute();
}

TEST_F(BasicIdleHub, inhibit_idle_remains_inhibited)
{
    // TODO - write
}

TEST_F(BasicIdleHub, resume_idle_after_inhibited)
{
    // TODO - write
}

TEST_F(BasicIdleHub, resume_idle_not_inhibited)
{
    // TODO - write
}
