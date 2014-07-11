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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "mir/basic_observers.h"
#include "mir_test/wait_condition.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace
{

struct DummyObserver
{
};

struct DummyObservers : mir::BasicObservers<DummyObserver>
{
    using mir::BasicObservers<DummyObserver>::add;
    using mir::BasicObservers<DummyObserver>::remove;
    using mir::BasicObservers<DummyObserver>::for_each;
};

struct BasicObserversTest : testing::Test
{
    DummyObservers observers;

    std::shared_ptr<DummyObserver> const observer1 = std::make_shared<DummyObserver>();
    std::shared_ptr<DummyObserver> const observer2 = std::make_shared<DummyObserver>();
};

}

TEST_F(BasicObserversTest, can_remove_observer_from_within_same_observer)
{
    using namespace testing;

    observers.add(observer1);

    observers.for_each(
        [&] (std::shared_ptr<DummyObserver> const& observer)
        {
            observers.remove(observer);
        });

    int observers_seen = 0;

    observers.for_each(
        [&] (std::shared_ptr<DummyObserver> const&)
        {
            ++observers_seen;
        });

    EXPECT_THAT(observers_seen, Eq(0));
}

TEST_F(BasicObserversTest, can_remove_unused_observer_from_within_different_observer)
{
    using namespace testing;

    observers.add(observer1);
    observers.add(observer2);

    int observers_seen = 0;

    observers.for_each(
        [&] (std::shared_ptr<DummyObserver> const&)
        {
            observers.remove(observer2);
            ++observers_seen;
        });

    EXPECT_THAT(observers_seen, Eq(1));
}

TEST_F(BasicObserversTest,
       can_remove_unused_observer_while_different_observer_is_used_in_different_thread)
{
    using namespace testing;

    observers.add(observer1);
    observers.add(observer2);

    mir::test::WaitCondition first_observer_in_use;
    mir::test::WaitCondition second_observer_removed;

    int observers_seen = 0;

    std::thread t{
        [&]
        {
            observers.for_each(
                [&] (std::shared_ptr<DummyObserver> const&)
                {
                    first_observer_in_use.wake_up_everyone();
                    second_observer_removed.wait_for_at_most_seconds(3);
                    EXPECT_TRUE(second_observer_removed.woken());
                    ++observers_seen;
                });
        }};

    first_observer_in_use.wait_for_at_most_seconds(3);
    observers.remove(observer2);
    second_observer_removed.wake_up_everyone();

    t.join();

    EXPECT_THAT(observers_seen, Eq(1));
}
