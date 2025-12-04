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
#include <thread>
#include <atomic>

#include <mir/executor.h>
#include <mir/test/signal.h>

using namespace std::literals::chrono_literals;
using namespace testing;

namespace mt = mir::test;

TEST(LinearisingExecutor, executes_work)
{
    auto done = std::make_shared<mt::Signal>();
    mir::linearising_executor.spawn([done]() { done->raise(); });

    EXPECT_TRUE(done->wait_for(60s));
}

TEST(LinearisingExecutor, does_not_execute_concurrently)
{
    std::atomic<int> counter{0};
    std::atomic<int> which_thread{0};

    // Should be make_shared<mt::Signal[2]>, but as of 22.04 g++/libstdc++ don't implement that bit of C++20
    std::shared_ptr<mt::Signal[2]> done{new mt::Signal[2]};

    auto counter_check =
        [&counter, &which_thread, done]()
        {
            EXPECT_THAT(counter, Eq(0));
            counter++;
            std::this_thread::sleep_for(1s);
            EXPECT_THAT(counter, Eq(1));
            counter--;
            done[which_thread++].raise();
        };

    mir::linearising_executor.spawn(counter_check);
    mir::linearising_executor.spawn(counter_check);

    done[0].wait_for(60s);
    done[1].wait_for(60s);
}

TEST(LinearisingExecutor, happens_before_is_executed_before)
{
    std::atomic<int> counter{0};
    constexpr int expected_count = 2;
    auto done = std::make_shared<mt::Signal>();

    for (int i = 0; i < expected_count; ++i)
    {
        mir::linearising_executor.spawn(
            [&counter]()
            {
                std::this_thread::sleep_for(1s);
                ++counter;
            });
    }
    mir::linearising_executor.spawn([done]() { done->raise(); });

    ASSERT_TRUE(done->wait_for(60s));
    EXPECT_THAT(counter, Eq(expected_count));
}

TEST(LinearisingExecutor, spawns_work_on_different_thread)
{
    auto this_thread_done = std::make_shared<mt::Signal>();
    auto work_done = std::make_shared<mt::Signal>();

    mir::linearising_executor.spawn(
        [this_thread_done, work_done]()
        {
            EXPECT_TRUE(this_thread_done->wait_for(60s));
            work_done->raise();
        });

    this_thread_done->raise();
    EXPECT_TRUE(work_done->wait_for(60s));
}
