/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored by: Alberto Aguirre <alberto.aguirre@canonical.com>
 */

#include "mir/thread/basic_thread_pool.h"

#include "mir/thread_name.h"
#include "mir/test/current_thread_name.h"
#include "mir/test/signal.h"

#include <atomic>
#include <memory>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mt = mir::test;
namespace mth = mir::thread;

namespace
{
class BasicThreadPool : public testing::Test
{
public:
    BasicThreadPool()
        : default_task_id{nullptr},
          default_num_threads{3},
          expected_name{"test_thread"}
    {
    }
protected:
    mth::BasicThreadPool::TaskId default_task_id;
    int default_num_threads;
    std::string expected_name;
};

class TestTask
{
public:
    TestTask()
        : TestTask("")
    {
    }

    TestTask(std::string name)
        : called{false},
          block{false},
          name{name}
    {
    }

    ~TestTask()
    {
        unblock();
    }

    void operator()() noexcept
    {
        called = true;
        if (!name.empty())
            mir::set_thread_name(name);
#ifndef MIR_DONT_USE_PTHREAD_GETNAME_NP
        else
            name = mt::current_thread_name();
#endif
        if (block)
            signal.wait();
    }

    std::string thread_name() const
    {
        return name;
    }

    bool was_called() const
    {
        return called;
    }

    void block_on_execution()
    {
        block = true;
    }

    void unblock()
    {
        signal.raise();
    }

private:
    std::atomic<bool> called;
    bool block;
    std::string name;
    mt::Signal signal;
};
}

TEST_F(BasicThreadPool, executes_given_functor)
{
    using namespace testing;
    mth::BasicThreadPool p{default_num_threads};

    TestTask task;
    auto future = p.run(std::ref(task));
    future.wait();

    EXPECT_TRUE(task.was_called());
}

#ifndef MIR_DONT_USE_PTHREAD_GETNAME_NP
TEST_F(BasicThreadPool, executes_on_preferred_thread)
#else
TEST_F(BasicThreadPool, DISABLED_executes_on_preferred_thread)
#endif
{
    using namespace testing;
    mth::BasicThreadPool p{default_num_threads};

    TestTask task1{expected_name};
    task1.block_on_execution();
    auto future1 = p.run(std::ref(task1), default_task_id);

    // This task should be queued to the thread associated with the given id
    // even when its busy
    TestTask task2;
    auto future2 = p.run(std::ref(task2), default_task_id);

    task1.unblock();
    future1.wait();
    future2.wait();

    // Check if the second task executed on the same thread as the first task
    EXPECT_TRUE(task1.was_called());
    EXPECT_TRUE(task2.was_called());
    EXPECT_THAT(task2.thread_name(), Eq(expected_name));
}

#ifndef MIR_DONT_USE_PTHREAD_GETNAME_NP
TEST_F(BasicThreadPool, executes_recycles_threads)
#else
TEST_F(BasicThreadPool, DISABLED_recycles_threads)
#endif
{
    using namespace testing;
    mth::BasicThreadPool p{2};

    std::string const thread1_name = "thread1";
    std::string const thread2_name = "thread2";

    //Create a couple of blocking tasks, so that we force the pool
    //to assign new threads to each task
    TestTask task1{thread1_name};
    task1.block_on_execution();
    auto future1 = p.run(std::ref(task1));

    TestTask task2{thread2_name};
    task2.block_on_execution();
    auto future2 = p.run(std::ref(task2));

    task1.unblock();
    task2.unblock();
    future1.wait();
    future2.wait();

    // Now we should have some idle threads, the next task should
    // run in any of the two previously created threads
    TestTask task3;
    auto future = p.run(std::ref(task3));
    future.wait();

    EXPECT_TRUE(task1.was_called());
    EXPECT_TRUE(task2.was_called());
    EXPECT_TRUE(task3.was_called());
    EXPECT_THAT(task3.thread_name(), AnyOf(Eq(thread1_name), Eq(thread2_name)));
}

TEST_F(BasicThreadPool, creates_new_threads)
{
    using namespace testing;
    mth::BasicThreadPool p{1};

    TestTask task1{expected_name};
    task1.block_on_execution();
    auto future1 = p.run(std::ref(task1));

    TestTask task2;
    auto future2 = p.run(std::ref(task2));

    task1.unblock();
    future1.wait();
    future2.wait();

    EXPECT_TRUE(task1.was_called());
    EXPECT_TRUE(task2.was_called());
    EXPECT_THAT(task2.thread_name(), Ne(expected_name));
}

TEST_F(BasicThreadPool, can_shrink)
{
    using namespace testing;
    mth::BasicThreadPool p{0};

    TestTask task1{expected_name};
    auto future = p.run(std::ref(task1));
    future.wait();

    // This should delete all threads since we specified a minimum of 0 threads for our pool
    p.shrink();

    TestTask task2;
    future = p.run(std::ref(task2));
    future.wait();

    EXPECT_TRUE(task1.was_called());
    EXPECT_TRUE(task2.was_called());
    EXPECT_THAT(task2.thread_name(), Ne(expected_name));
}
