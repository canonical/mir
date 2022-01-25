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
 *
 * Authored by: William Wold <william.wold@canonical.com>
 */

#include "src/server/scene/basic_idle_hub.h"
#include "mir/test/doubles/advanceable_clock.h"
#include "mir/test/doubles/fake_timer.h"
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
    mtd::FakeTimer timer{mt::fake_shared(clock)};
    ms::BasicIdleHub hub{mt::fake_shared(clock), timer};
};
}

TEST_F(BasicIdleHub, observer_marked_active_initially)
{
    auto const observer = std::make_shared<StrictMock<MockObserver>>();
    EXPECT_CALL(*observer, active());
    hub.register_interest(observer, 5s);
}

TEST_F(BasicIdleHub, observer_marked_idle_initially_after_wait)
{
    auto const observer = std::make_shared<StrictMock<MockObserver>>();
    clock.advance_by(6s);
    EXPECT_CALL(*observer, idle());
    hub.register_interest(observer, 5s);
}

TEST_F(BasicIdleHub, observer_marked_active_initially_after_wait_and_poke)
{
    auto const observer = std::make_shared<StrictMock<MockObserver>>();
    clock.advance_by(6s);
    hub.poke();
    clock.advance_by(4s);
    EXPECT_CALL(*observer, active());
    hub.register_interest(observer, 5s);
}

TEST_F(BasicIdleHub, observer_marked_idle_after_wait)
{
    auto const observer = std::make_shared<StrictMock<MockObserver>>();
    EXPECT_CALL(*observer, active());
    hub.register_interest(observer, 5s);
    clock.advance_by(4s);
    EXPECT_CALL(*observer, idle());
    clock.advance_by(6s);
}

TEST_F(BasicIdleHub, observer_marked_active_after_poke)
{
    auto const observer = std::make_shared<StrictMock<MockObserver>>();
    EXPECT_CALL(*observer, active());
    hub.register_interest(observer, 5s);
    EXPECT_CALL(*observer, idle());
    clock.advance_by(6s);
    EXPECT_CALL(*observer, active());
    hub.poke();
}

TEST_F(BasicIdleHub, observer_can_remove_itself_in_idle_notification)
{
    auto const observer = std::make_shared<StrictMock<MockObserver>>();
    EXPECT_CALL(*observer, active());
    hub.register_interest(observer, 5s);
    EXPECT_CALL(*observer, idle()).WillOnce(Invoke([&]()
        {
            hub.unregister_interest(*observer);
        }));
    clock.advance_by(6s);
}

TEST_F(BasicIdleHub, second_observer_with_same_timeout_marked_idle_initially_after_wait)
{
    auto const observer1 = std::make_shared<NiceMock<MockObserver>>();
    auto const observer2 = std::make_shared<StrictMock<MockObserver>>();
    hub.register_interest(observer1, 5s);
    clock.advance_by(6s);
    EXPECT_CALL(*observer2, idle());
    hub.register_interest(observer2, 5s);
}

TEST_F(BasicIdleHub, second_observer_with_same_timeout_marked_active_initially_after_wait_and_poke)
{
    auto const observer1 = std::make_shared<NiceMock<MockObserver>>();
    auto const observer2 = std::make_shared<StrictMock<MockObserver>>();
    hub.register_interest(observer1, 5s);
    clock.advance_by(6s);
    hub.poke();
    EXPECT_CALL(*observer2, active());
    hub.register_interest(observer2, 5s);
}

TEST_F(BasicIdleHub, multiple_observers_with_same_timeout_marked_idle_after_wait)
{
    auto const observer1 = std::make_shared<StrictMock<MockObserver>>();
    auto const observer2 = std::make_shared<StrictMock<MockObserver>>();
    auto const observer3 = std::make_shared<StrictMock<MockObserver>>();
    EXPECT_CALL(*observer1, active());
    EXPECT_CALL(*observer2, active());
    EXPECT_CALL(*observer3, active());
    hub.register_interest(observer1, 5s);
    hub.register_interest(observer2, 5s);
    hub.register_interest(observer3, 5s);
    EXPECT_CALL(*observer1, idle());
    EXPECT_CALL(*observer2, idle());
    EXPECT_CALL(*observer3, idle());
    clock.advance_by(6s);
}

TEST_F(BasicIdleHub, multiple_observers_with_different_timeouts_marked_idle_at_correct_time)
{
    auto const observer1 = std::make_shared<StrictMock<MockObserver>>();
    auto const observer2 = std::make_shared<StrictMock<MockObserver>>();
    auto const observer3 = std::make_shared<StrictMock<MockObserver>>();
    EXPECT_CALL(*observer1, active());
    EXPECT_CALL(*observer2, active());
    EXPECT_CALL(*observer3, active());
    hub.register_interest(observer1, 5s);
    hub.register_interest(observer2, 10s);
    hub.register_interest(observer3, 12s);
    EXPECT_CALL(*observer1, idle());
    clock.advance_by(6s);
    EXPECT_CALL(*observer2, idle());
    clock.advance_by(5s);
    EXPECT_CALL(*observer3, idle());
    clock.advance_by(2s);
}

TEST_F(BasicIdleHub, multiple_observers_with_different_timeouts_marked_active_after_poke)
{
    auto const observer1 = std::make_shared<StrictMock<MockObserver>>();
    auto const observer2 = std::make_shared<StrictMock<MockObserver>>();
    auto const observer3 = std::make_shared<StrictMock<MockObserver>>();
    EXPECT_CALL(*observer1, active());
    EXPECT_CALL(*observer2, active());
    EXPECT_CALL(*observer3, active());
    hub.register_interest(observer1, 5s);
    hub.register_interest(observer2, 10s);
    hub.register_interest(observer3, 12s);
    EXPECT_CALL(*observer1, idle());
    EXPECT_CALL(*observer2, idle());
    EXPECT_CALL(*observer3, idle());
    clock.advance_by(13s);
    EXPECT_CALL(*observer1, active());
    EXPECT_CALL(*observer2, active());
    EXPECT_CALL(*observer3, active());
    hub.poke();
}

TEST_F(BasicIdleHub, observer_marked_idle_after_observer_with_same_timeout_removed)
{
    auto const observer1 = std::make_shared<StrictMock<MockObserver>>();
    auto const observer2 = std::make_shared<StrictMock<MockObserver>>();
    EXPECT_CALL(*observer1, active());
    EXPECT_CALL(*observer2, active());
    hub.register_interest(observer1, 5s);
    hub.register_interest(observer2, 5s);
    hub.unregister_interest(*observer1);
    EXPECT_CALL(*observer2, idle());
    clock.advance_by(6s);
}

TEST_F(BasicIdleHub, observer_marked_idle_after_observer_with_shorter_timeout_removed)
{
    auto const observer1 = std::make_shared<StrictMock<MockObserver>>();
    auto const observer2 = std::make_shared<StrictMock<MockObserver>>();
    EXPECT_CALL(*observer1, active());
    EXPECT_CALL(*observer2, active());
    hub.register_interest(observer1, 5s);
    hub.register_interest(observer2, 10s);
    hub.unregister_interest(*observer1);
    EXPECT_CALL(*observer2, idle());
    clock.advance_by(11s);
}

TEST_F(BasicIdleHub, observer_marked_idle_when_added_after_observer_with_longer_timeout)
{
    auto const observer1 = std::make_shared<StrictMock<MockObserver>>();
    auto const observer2 = std::make_shared<StrictMock<MockObserver>>();
    EXPECT_CALL(*observer1, active());
    EXPECT_CALL(*observer2, active());
    hub.register_interest(observer1, 10s);
    hub.register_interest(observer2, 5s);
    hub.unregister_interest(*observer1);
    EXPECT_CALL(*observer2, idle());
    clock.advance_by(6s);
}

TEST_F(BasicIdleHub, observer_marked_idle_after_shorter_timeout_removed_and_poke)
{
    auto const observer1 = std::make_shared<StrictMock<MockObserver>>();
    auto const observer2 = std::make_shared<StrictMock<MockObserver>>();
    EXPECT_CALL(*observer1, active());
    EXPECT_CALL(*observer2, active());
    hub.register_interest(observer1, 5s);
    hub.register_interest(observer2, 10s);
    EXPECT_CALL(*observer1, idle());
    clock.advance_by(6s);
    hub.unregister_interest(*observer1);
    hub.poke();
    clock.advance_by(9s);
    EXPECT_CALL(*observer2, idle());
    clock.advance_by(2s);
}
