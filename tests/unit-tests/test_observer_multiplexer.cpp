/*
 * Copyright © 2016 Canonical Ltd.
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
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include "mir/observer_multiplexer.h"

#include "mir/test/barrier.h"
#include "mir/test/auto_unblock_thread.h"

#include <memory>
#include <array>
#include <thread>
#include <atomic>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mt = mir::test;

namespace
{
class TestObserver
{
public:
    virtual ~TestObserver() = default;
    virtual void observation_made(std::string const& arg) = 0;
};

class MockObserver : public TestObserver
{
public:
    MOCK_METHOD1(observation_made, void(std::string const&));
};

class TestObserverMultiplexer : public mir::ObserverMultiplexer<TestObserver>
{
public:
    void observation_made(std::string const& arg) override
    {
        for_each_observer([&arg](auto& observer) { observer.observation_made(arg); });
    }
};
}

TEST(ObserverMultiplexer, each_added_observer_recieves_observations)
{
    using namespace testing;
    std::string const value = "Hello, my name is Inigo Montoya.";

    auto observer_one = std::make_shared<NiceMock<MockObserver>>();
    auto observer_two = std::make_shared<NiceMock<MockObserver>>();

    EXPECT_CALL(*observer_one, observation_made(StrEq(value))).Times(2);
    EXPECT_CALL(*observer_two, observation_made(StrEq(value))).Times(1);

    TestObserverMultiplexer multiplexer;

    multiplexer.register_interest(observer_one);
    multiplexer.observation_made(value);

    multiplexer.register_interest(observer_two);
    multiplexer.observation_made(value);
}

TEST(ObserverMultiplexer, removed_observers_do_not_recieve_observations)
{
    using namespace testing;
    std::string const value = "The girl of my dreams is giving me nightmares";

    auto observer_one = std::make_shared<NiceMock<MockObserver>>();
    auto observer_two = std::make_shared<NiceMock<MockObserver>>();

    EXPECT_CALL(*observer_one, observation_made(StrEq(value))).Times(2);
    EXPECT_CALL(*observer_two, observation_made(StrEq(value))).Times(1);

    TestObserverMultiplexer multiplexer;

    multiplexer.register_interest(observer_one);
    multiplexer.register_interest(observer_two);
    multiplexer.observation_made(value);

    multiplexer.unregister_interest(*observer_two);
    multiplexer.observation_made(value);
}

TEST(ObserverMultiplexer, can_remove_observer_from_callback)
{
    using namespace testing;
    std::string const value = "Goldfinger";

    auto observer_one = std::make_shared<NiceMock<MockObserver>>();
    auto observer_two = std::make_shared<NiceMock<MockObserver>>();

    TestObserverMultiplexer multiplexer;

    EXPECT_CALL(*observer_one, observation_made(StrEq(value)))
        .WillOnce(Invoke(
            [observer_one = observer_one.get(), &multiplexer](auto)
            {
                multiplexer.unregister_interest(*observer_one);
            }));
    EXPECT_CALL(*observer_two, observation_made(StrEq(value)))
        .Times(2);

    multiplexer.register_interest(observer_one);
    multiplexer.register_interest(observer_two);

    multiplexer.observation_made(value);
    multiplexer.observation_made(value);
}

TEST(ObserverMultiplexer, multiple_threads_can_simultaneously_make_observations)
{
    using namespace testing;

    std::array<bool, 10> values_seen;
    std::array<std::string, values_seen.size()> values;
    for (auto i = 0u; i < values.size(); ++i)
    {
        values[i] = std::to_string(i);
    }

    auto observer = std::make_shared<NiceMock<MockObserver>>();
    ON_CALL(*observer, observation_made(_))
        .WillByDefault(
            Invoke(
                [&values, &values_seen](auto const& value)
                {
                    for (auto i = 0u; i < values.size(); ++i)
                    {
                        if (value == values[i])
                        {
                            values_seen[i] = true;
                        }
                    }
                }));

    TestObserverMultiplexer multiplexer;
    multiplexer.register_interest(observer);

    mt::Barrier threads_ready(values.size());
    mt::Barrier observations_made(values.size() + 1);
    std::array<mt::AutoJoinThread, values_seen.size()> threads;

    for (auto i = 0u; i < values.size(); ++i)
    {
        threads[i] = mt::AutoJoinThread(
            [&threads_ready, &observations_made, &multiplexer, value = values[i]]()
            {
                threads_ready.ready();
                multiplexer.observation_made(value);
                observations_made.ready();
            });
    }
    observations_made.ready();


    EXPECT_THAT(
        std::vector<bool>(values_seen.begin(), values_seen.end()),
        ContainerEq(std::vector<bool>(values_seen.size(), true)));
}

TEST(ObserverMultiplexer, multiple_threads_registering_unregistering_and_observing)
{
    using namespace testing;

    TestObserverMultiplexer multiplexer;

    auto observer_one = std::make_shared<NiceMock<MockObserver>>();
    auto observer_two = std::make_shared<NiceMock<MockObserver>>();
    ON_CALL(*observer_one, observation_made(_))
        .WillByDefault(
            Invoke(
                [&multiplexer, observer_two](std::string const& value)
                {
                    if (!value.compare("3"))
                    {
                        multiplexer.unregister_interest(*observer_two);
                    }
                    if (!value.compare("5"))
                    {
                        multiplexer.register_interest(observer_two);
                    }
                }));
    ON_CALL(*observer_one, observation_made(_))
        .WillByDefault(
            Invoke(
                [&multiplexer, observer_two](std::string const& value)
                {
                    if (!value.compare("8"))
                    {
                        multiplexer.unregister_interest(*observer_two);
                    }
                }));

    multiplexer.register_interest(observer_two);
    multiplexer.register_interest(observer_one);

    std::array<mt::AutoJoinThread, 10> threads;
    mt::Barrier threads_done(threads.size() + 1);

    for (auto i = 0u; i < threads.size(); ++i)
    {
        threads[i] = mt::AutoJoinThread(
            [&threads_done, &multiplexer]()
            {
                for (auto i = 0; i < 50; ++i)
                {
                    for (auto j = 0; j < 10; ++j)
                    {
                        multiplexer.observation_made(std::to_string(j));
                    }
                }
                threads_done.ready();
            });
    }
    threads_done.ready();
}

TEST(ObserverMultiplexer, multiple_threads_unregistering_same_observer_is_safe)
{
    using namespace testing;

    TestObserverMultiplexer multiplexer;

    auto observer_one = std::make_shared<NiceMock<MockObserver>>();
    auto observer_two = std::make_shared<NiceMock<MockObserver>>();

    std::atomic<int> call_count{0};
    ON_CALL(*observer_one, observation_made(_))
        .WillByDefault(
            Invoke(
                [&multiplexer, observer_two, &call_count](auto)
                {
                    ++call_count;
                    multiplexer.unregister_interest(*observer_two);
                }));
    std::atomic<bool> victim_called{false};
    ON_CALL(*observer_two, observation_made(_))
        .WillByDefault(Invoke([&victim_called](auto) { victim_called = true; }));

    multiplexer.register_interest(observer_one);
    multiplexer.register_interest(observer_two);

    std::array<mt::AutoJoinThread, 10> threads;
    mt::Barrier threads_done(threads.size() + 1);

    for (auto i = 0u; i < threads.size(); ++i)
    {
        threads[i] = mt::AutoJoinThread(
            [&threads_done, &multiplexer]()
            {
                multiplexer.observation_made("Hello");
                threads_done.ready();
            });
    }
    threads_done.ready();

    auto precount = call_count.load();

    multiplexer.observation_made("Banana");

    EXPECT_THAT(call_count, Eq(precount + 1));
}

TEST(ObserverMultiplexer, registering_is_threadsafe)
{
    using namespace testing;

    std::array<bool, 100> observer_notified;
    std::array<std::shared_ptr<NiceMock<MockObserver>>, observer_notified.size()> observers;

    for (auto i = 0u; i < observers.size(); ++i)
    {
        observers[i] = std::make_shared<NiceMock<MockObserver>>();
        ON_CALL(*observers[i], observation_made(_))
            .WillByDefault(Invoke([notified = &observer_notified[i]](auto) { *notified = true; }));
    }
    std::array<mt::AutoJoinThread, observer_notified.size()> threads;
    mt::Barrier threads_done(threads.size() + 1);

    TestObserverMultiplexer multiplexer;

    for (auto i = 0u; i < threads.size(); ++i)
    {
        threads[i] = mt::AutoJoinThread(
            [&threads_done, &multiplexer, observer = observers[i]]()
            {
                multiplexer.register_interest(observer);
                threads_done.ready();
            });
    }
    threads_done.ready();

    multiplexer.observation_made("Foo");

    EXPECT_THAT(
        std::vector<bool>(observer_notified.begin(), observer_notified.end()),
        ContainerEq(std::vector<bool>(observer_notified.size(), true)));
}

TEST(ObserverMultiplexer, unregistering_is_threadsafe)
{
    using namespace testing;

    std::array<bool, 10> observer_notified;
    for (auto& notified : observer_notified)
    {
        notified = false;
    }

    std::array<std::shared_ptr<NiceMock<MockObserver>>, observer_notified.size()> observers;

    for (auto i = 0u; i < observers.size(); ++i)
    {
        observers[i] = std::make_shared<NiceMock<MockObserver>>();
        ON_CALL(*observers[i], observation_made(_))
            .WillByDefault(Invoke([notified = &observer_notified[i]](auto) { *notified = true; }));
    }
    std::array<mt::AutoJoinThread, observer_notified.size()> threads;
    mt::Barrier threads_done(threads.size() + 1);

    TestObserverMultiplexer multiplexer;

    for (auto observer : observers)
    {
        multiplexer.register_interest(observer);
    }

    for (auto i = 0u; i < threads.size(); ++i)
    {
        threads[i] = mt::AutoJoinThread(
            [&threads_done, &multiplexer, observer = observers[i]]()
        {
            multiplexer.unregister_interest(*observer);
            threads_done.ready();
        });
    }
    threads_done.ready();

    multiplexer.observation_made("Foo");

    EXPECT_THAT(
        std::vector<bool>(observer_notified.begin(), observer_notified.end()),
        ContainerEq(std::vector<bool>(observer_notified.size(), false)));
}

TEST(ObserverMultiplexer, can_trigger_observers_from_observers)
{
    using namespace testing;
    constexpr char const* first_observation = "Elementary, my dear Watson";
    constexpr char const* second_observation = "You're one microscopic cog in his catastrophic plan";

    TestObserverMultiplexer multiplexer;

    auto observer = std::make_shared<NiceMock<MockObserver>>();
    EXPECT_CALL(*observer, observation_made(StrEq(first_observation)))
        .WillOnce(InvokeWithoutArgs([&multiplexer]() { multiplexer.observation_made(second_observation); }));
    EXPECT_CALL(*observer, observation_made(StrEq(second_observation)));

    multiplexer.register_interest(observer);

    multiplexer.observation_made(first_observation);
}