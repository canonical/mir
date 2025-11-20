/*
 * Copyright © Canonical Ltd.
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
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <boost/throw_exception.hpp>
#include <chrono>
#include <thread>
#include <future>

#include <mir/executor.h>
#include <mir/test/signal.h>
#include <mir/test/current_thread_name.h>

using namespace std::literals::chrono_literals;
using namespace testing;

namespace mt = mir::test;

TEST(ThreadPoolExecutor, executes_work)
{
    auto const done = std::make_shared<mt::Signal>();
    mir::thread_pool_executor.spawn([done]() { done->raise(); });

    EXPECT_TRUE(done->wait_for(60s));
}

TEST(ThreadPoolExecutor, can_execute_work_from_within_work_item)
{
    auto const done = std::make_shared<mt::Signal>();

    mir::thread_pool_executor.spawn(
        [done]()
        {
            mir::thread_pool_executor.spawn([done]() { done->raise(); });
        });

    EXPECT_TRUE(done->wait_for(60s));
}

TEST(ThreadPoolExecutor, work_executed_from_within_work_item_is_not_blocked_by_work_item_blocking)
{
    auto const done = std::make_shared<mt::Signal>();
    auto const waited_for_done = std::make_shared<mt::Signal>();

    mir::thread_pool_executor.spawn(
        [done, waited_for_done]()
        {
            mir::thread_pool_executor.spawn([done]() { done->raise(); });
            if (!waited_for_done->wait_for(60s))
            {
                FAIL() << "Spawned work failed to execute";
            }
        });

    EXPECT_TRUE(done->wait_for(60s));
    waited_for_done->raise();
}

TEST(ThreadPoolExecutorDeathTest, unhandled_exception_in_work_item_causes_termination_by_default)
{
    EXPECT_DEATH(
        {
            mir::thread_pool_executor.spawn(
                []()
                {
                    BOOST_THROW_EXCEPTION((std::runtime_error{"Oops, unhandled exception"}));
                });
            std::this_thread::sleep_for(std::chrono::seconds{60});
        },
        ""
    );
}

TEST(ThreadPoolExecutor, can_set_unhandled_exception_handler)
{
    static std::promise<std::exception_ptr> exception_pipe;

    auto exception = exception_pipe.get_future();

    mir::ThreadPoolExecutor::set_unhandled_exception_handler(
        []()
        {
            exception_pipe.set_value(std::current_exception());
        });

    mir::thread_pool_executor.spawn([]() { throw std::runtime_error{"Boop!"}; });

    EXPECT_THAT(exception.wait_for(std::chrono::seconds{60}), Eq(std::future_status::ready));

    try
    {
        std::rethrow_exception(exception.get());
    }
    catch (std::runtime_error const& err)
    {
        EXPECT_THAT(err.what(), StrEq("Boop!"));
    }
}

#ifndef MIR_DONT_USE_PTHREAD_GETNAME_NP
TEST(ThreadPoolExecutor, executor_threads_have_sensible_name)
#else
TEST(ThreadPoolExecutor, DISABLED_executor_threads_have_sensible_names)
#endif
{
    auto thread_name_provider = std::make_shared<std::promise<std::string>>();
    auto thread_name = thread_name_provider->get_future();

    mir::thread_pool_executor.spawn(
        [thread_name_provider]()
        {
            thread_name_provider->set_value(mt::current_thread_name());
        });

    ASSERT_THAT(thread_name.wait_for(std::chrono::seconds{60}), Eq(std::future_status::ready));
    EXPECT_THAT(thread_name.get(), MatchesRegex("Mir/Workqueue.*"));
}

TEST(ThreadPoolExecutor, work_with_dependencies_completes)
{
    std::array<std::shared_ptr<std::promise<void>>, 100> promises;
    for (auto& promise : promises)
    {
        promise = std::make_shared<std::promise<void>>();
    }

    // Set up a big chain of work, with each item depending on the one after it.
    for(auto i = 0; i < promises.size() - 1; ++i)
    {
        mir::thread_pool_executor.spawn(
            [wait_on_promise = promises[i + 1], signal_next = promises[i]]()
            {
                auto wait_on = wait_on_promise->get_future();
                if (wait_on.wait_for(std::chrono::seconds{60}) != std::future_status::ready)
                {
                    ADD_FAILURE() << "Timeout waiting for previous work to signal";
                    throw std::runtime_error{"Timeout"};
                }
                signal_next->set_value();
            });
    }

    auto first_work_completed = promises[0]->get_future();

    // Release the chain by triggering the final work item.
    promises.back()->set_value();

    EXPECT_THAT(first_work_completed.wait_for(std::chrono::seconds{60}), Eq(std::future_status::ready));
}

TEST(ThreadPoolExecutor, new_work_can_be_submitted_after_quiesce)
{
    auto done = std::make_shared<mt::Signal>();
    constexpr int const work_count{10};
    std::atomic<int> work_index{0};

    for (auto i = 0; i < work_count; ++i)
    {
        mir::thread_pool_executor.spawn(
            [&work_index, done]()
            {
                if (++work_index == work_count)
                {
                    done->raise();
                }
            });
    }

    ASSERT_TRUE(done->wait_for(60s));
    mir::ThreadPoolExecutor::quiesce();
    done->reset();

    work_index = 0;
    for (auto i = 0; i < work_count; ++i)
    {
        mir::thread_pool_executor.spawn(
            [&work_index, done]()
            {
                if (++work_index == work_count)
                {
                    done->raise();
                }
            });
    }
    EXPECT_TRUE(done->wait_for(60s));
}

TEST(ThreadPoolExecutor, quiesce_waits_until_work_completes)
{
    constexpr auto const delay = 500ms;

    auto const expected_end = std::chrono::steady_clock::now() + delay;

    mir::thread_pool_executor.spawn(
        [delay]()
        {
            std::this_thread::sleep_for(delay);
        });

    mir::ThreadPoolExecutor::quiesce();
    EXPECT_THAT(std::chrono::steady_clock::now(), Gt(expected_end));
}
