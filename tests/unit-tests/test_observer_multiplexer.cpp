/*
 * Copyright © 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
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
#include <queue>

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
    virtual void multi_argument_observation(std::string const& arg, int another_one, float third) = 0;
};

class MockObserver : public TestObserver
{
public:
    MOCK_METHOD1(observation_made, void(std::string const&));
    MOCK_METHOD3(multi_argument_observation, void(std::string const&, int, float));
};

class ThreadedExecutor : public mir::Executor
{
public:
    ThreadedExecutor()
    {
        worker = std::thread{
            [this]()
            {
                while (running)
                {
                    std::function<void()> work{nullptr};
                    {
                        std::unique_lock<decltype(work_mutex)> lock{work_mutex};
                        work_changed.wait(lock, [this]() { return (work_pending.size() > 0) || !running; });

                        if (work_pending.size() > 0)
                        {
                            work = work_pending.front();
                        }
                    }

                    if (work)
                    {
                        work();

                        {
                            std::lock_guard<decltype(work_mutex)> lock{work_mutex};
                            work_pending.pop();
                            work_changed.notify_all();
                        }
                        work_changed.notify_all();
                    }
                }
            }
        };
    }

    void drain_work()
    {
        std::unique_lock<decltype(work_mutex)> lock{work_mutex};
        work_changed.wait(lock, [this]() { return work_pending.size() == 0; });
    }

    ~ThreadedExecutor()
    {
        drain_work();

        running = false;
        work_changed.notify_all();

        worker.join();
    }

    void spawn(std::function<void()>&& work) override
    {
        std::lock_guard<decltype(work_mutex)> lock{work_mutex};
        work_pending.emplace(std::move(work));
        work_changed.notify_all();
    }

private:
    std::thread worker;
    std::atomic<bool> running{true};

    std::mutex work_mutex;
    std::condition_variable work_changed;
    std::queue<std::function<void()>> work_pending;
};

class TestObserverMultiplexer : public mir::ObserverMultiplexer<TestObserver>
{
public:
    TestObserverMultiplexer(mir::Executor& executor)
        : ObserverMultiplexer(executor)
    {
    }

    void observation_made(std::string const& arg) override
    {
        for_each_observer(&TestObserver::observation_made, arg);
    }

    void multi_argument_observation(std::string const& arg, int another_one, float third) override
    {
        for_each_observer(&TestObserver::multi_argument_observation, arg, another_one, third);
    }
};
}

TEST(ObserverMultiplexer, each_added_observer_recieves_observations)
{
    using namespace testing;
    ThreadedExecutor executor;
    TestObserverMultiplexer multiplexer{executor};
    std::string const value = "Hello, my name is Inigo Montoya.";


    auto observer_one = std::make_shared<NiceMock<MockObserver>>();
    auto observer_two = std::make_shared<NiceMock<MockObserver>>();

    EXPECT_CALL(*observer_one, observation_made(StrEq(value))).Times(2);
    EXPECT_CALL(*observer_two, observation_made(StrEq(value))).Times(1);


    multiplexer.register_interest(observer_one);
    multiplexer.observation_made(value);

    multiplexer.register_interest(observer_two);
    multiplexer.observation_made(value);

    executor.drain_work();
}

TEST(ObserverMultiplexer, removed_observers_do_not_recieve_observations)
{
    using namespace testing;
    std::string const value = "The girl of my dreams is giving me nightmares";

    ThreadedExecutor executor;
    TestObserverMultiplexer multiplexer{executor};

    auto observer_one = std::make_shared<NiceMock<MockObserver>>();
    auto observer_two = std::make_shared<NiceMock<MockObserver>>();

    EXPECT_CALL(*observer_one, observation_made(StrEq(value))).Times(2);
    EXPECT_CALL(*observer_two, observation_made(StrEq(value))).Times(1);

    multiplexer.register_interest(observer_one);
    multiplexer.register_interest(observer_two);
    multiplexer.observation_made(value);

    multiplexer.unregister_interest(*observer_two);
    multiplexer.observation_made(value);

    executor.drain_work();
}

TEST(ObserverMultiplexer, can_remove_observer_from_callback)
{
    using namespace testing;
    std::string const value = "Goldfinger";

    auto observer_one = std::make_shared<NiceMock<MockObserver>>();
    auto observer_two = std::make_shared<NiceMock<MockObserver>>();

    ThreadedExecutor executor;
    TestObserverMultiplexer multiplexer{executor};

    EXPECT_CALL(*observer_one, observation_made(StrEq(value)))
        .WillOnce(Invoke(
            [observer_one = observer_one.get(), &multiplexer, &value](auto)
            {
                multiplexer.unregister_interest(*observer_one);
                multiplexer.observation_made(value);
            }));
    EXPECT_CALL(*observer_two, observation_made(StrEq(value)))
        .Times(2);

    multiplexer.register_interest(observer_one);
    multiplexer.register_interest(observer_two);

    multiplexer.observation_made(value);

    executor.drain_work();
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

    ThreadedExecutor executor;
    TestObserverMultiplexer multiplexer{executor};
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

    executor.drain_work();

    EXPECT_THAT(
        std::vector<bool>(values_seen.begin(), values_seen.end()),
        ContainerEq(std::vector<bool>(values_seen.size(), true)));
}

TEST(ObserverMultiplexer, multiple_threads_registering_unregistering_and_observing)
{
    using namespace testing;

    auto observer_one = std::make_shared<NiceMock<MockObserver>>();
    auto observer_two = std::make_shared<NiceMock<MockObserver>>();

    ThreadedExecutor executor;
    TestObserverMultiplexer multiplexer{executor};

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

    executor.drain_work();
}

TEST(ObserverMultiplexer, multiple_threads_unregistering_same_observer_is_safe)
{
    using namespace testing;

    ThreadedExecutor executor;
    TestObserverMultiplexer multiplexer{executor};

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

    executor.drain_work();

    auto precount = call_count.load();

    multiplexer.observation_made("Banana");

    executor.drain_work();

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

    ThreadedExecutor executor;
    TestObserverMultiplexer multiplexer{executor};

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

    executor.drain_work();

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

    ThreadedExecutor executor;
    TestObserverMultiplexer multiplexer{executor};

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

    executor.drain_work();

    EXPECT_THAT(
        std::vector<bool>(observer_notified.begin(), observer_notified.end()),
        ContainerEq(std::vector<bool>(observer_notified.size(), false)));
}

TEST(ObserverMultiplexer, can_trigger_observers_from_observers)
{
    using namespace testing;
    constexpr char const* first_observation = "Elementary, my dear Watson";
    constexpr char const* second_observation = "You're one microscopic cog in his catastrophic plan";

    auto observer = std::make_shared<NiceMock<MockObserver>>();

    ThreadedExecutor executor;
    TestObserverMultiplexer multiplexer{executor};

    EXPECT_CALL(*observer, observation_made(StrEq(first_observation)))
        .WillOnce(InvokeWithoutArgs([&multiplexer]() { multiplexer.observation_made(second_observation); }));
    EXPECT_CALL(*observer, observation_made(StrEq(second_observation)));

    multiplexer.register_interest(observer);

    multiplexer.observation_made(first_observation);

    executor.drain_work();
}

TEST(ObserverMultiplexer, addition_takes_effect_immediately_even_in_callback)
{
    using namespace testing;
    constexpr char const* first_observation = "Rhythm & Blues Alibi";
    constexpr char const* second_observation = "Blue Moon Rising";

    ThreadedExecutor executor;
    TestObserverMultiplexer multiplexer{executor};

    auto observer_one = std::make_shared<NiceMock<MockObserver>>();
    auto observer_two = std::make_shared<NiceMock<MockObserver>>();

    EXPECT_CALL(*observer_one, observation_made(StrEq(first_observation)))
        .WillOnce(
            InvokeWithoutArgs(
                [&multiplexer, observer_two]()
                {
                    multiplexer.register_interest(observer_two);
                    multiplexer.observation_made(second_observation);
                }));
    EXPECT_CALL(*observer_one, observation_made(Not(StrEq(first_observation))))
        .Times(AnyNumber());
    EXPECT_CALL(*observer_two, observation_made(StrEq(second_observation)));

    multiplexer.register_interest(observer_one);

    multiplexer.observation_made(first_observation);

    executor.drain_work();
}

TEST(ObserverMultiplexer, observations_can_be_delegated_to_specified_executor)
{
    using namespace testing;

    class CountingExecutor : public mir::Executor
    {
    public:
        void spawn(std::function<void()>&& work) override
        {
            std::lock_guard<decltype(work_mutex)> lock{work_mutex};
            ++spawn_count;
            work_queue.emplace(std::move(work));
        }

        void do_work()
        {
            std::unique_lock<decltype(work_mutex)> lock{work_mutex};
            while (!work_queue.empty())
            {
                auto work = work_queue.front();
                work_queue.pop();
                lock.unlock();

                work();

                lock.lock();
            }
        }

        int work_spawned()
        {
            std::lock_guard<decltype(work_mutex)> lock{work_mutex};
            return spawn_count;
        }
    private:
        int spawn_count{0};

        std::mutex work_mutex;
        std::queue<std::function<void()>> work_queue;
    } counting_executor;

    ThreadedExecutor executor;
    TestObserverMultiplexer multiplexer{executor};

    auto observer_one = std::make_shared<NiceMock<MockObserver>>();
    auto observer_two = std::make_shared<NiceMock<MockObserver>>();

    EXPECT_CALL(*observer_one, observation_made(_));
    EXPECT_CALL(*observer_two, observation_made(_));

    multiplexer.register_interest(observer_one);
    multiplexer.register_interest(observer_two, counting_executor);

    EXPECT_THAT(counting_executor.work_spawned(), Eq(0));

    multiplexer.observation_made("Hello!");

    EXPECT_THAT(counting_executor.work_spawned(), Eq(1));

    counting_executor.do_work();
    executor.drain_work();
}

TEST(ObserverMultiplexer, multi_argument_observations_work)
{
    using namespace testing;

    std::string const a{"The Owls Go"};
    constexpr int b{55};
    constexpr float c{3.1415f};

    ThreadedExecutor executor;
    TestObserverMultiplexer multiplexer{executor};

    auto observer = std::make_shared<NiceMock<MockObserver>>();

    multiplexer.register_interest(observer);

    EXPECT_CALL(*observer, multi_argument_observation(a, b, c));

    multiplexer.multi_argument_observation(a, b, c);

    executor.drain_work();
}

TEST(ObserverMultiplexer, destroyed_observer_is_not_called)
{
    using namespace testing;

    class ExplicitExectutor : public mir::Executor
    {
    public:
        void spawn(std::function<void()>&& work) override
        {
            work_queue.push(std::move(work));
        }

        void run_work_items()
        {
            while(!work_queue.empty())
            {
                auto work_item = std::move(work_queue.front());
                work_queue.pop();
                work_item();
            }
        }
    private:
        std::queue<std::function<void()>> work_queue;
    };

    ExplicitExectutor executor;
    TestObserverMultiplexer multiplexer{executor};

    auto observer_owner = std::make_unique<NiceMock<MockObserver>>();
    // We need a shared_ptr that we can release, but we also need the observer to remain live.
    // So we construct a shared_ptr<> with an empty Deleter
    std::shared_ptr<MockObserver> observer{observer_owner.get(), [](auto){}};
    std::weak_ptr<MockObserver> observer_lifetime_observer{observer};

    multiplexer.register_interest(observer);
    EXPECT_CALL(*observer, observation_made(_)).Times(0);

    multiplexer.observation_made("the songs that we sung when the dark days come");
    observer.reset();

    // The test requires that our observer's lifetime has ended before we dispatch the observer
    ASSERT_THAT(observer_lifetime_observer.lock(), IsNull());
    // Run the executor; because the observer is dead, this should not dispatch to it.
    executor.run_work_items();
}
