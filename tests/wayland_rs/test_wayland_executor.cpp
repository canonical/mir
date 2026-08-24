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

#include "wayland_rs_server_test.h"

#include <gtest/gtest.h>

#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>

using namespace std::chrono_literals;

namespace
{
class WaylandExecutorTest : public mir::wayland::test::RunningWaylandServerTest
{
};
}

TEST_F(WaylandExecutorTest, runs_scheduled_work)
{
    std::mutex mutex;
    std::condition_variable cv;
    bool ran{false};

    executor->spawn(
        [&]
        {
            std::lock_guard<std::mutex> lock{mutex};
            ran = true;
            cv.notify_one();
        });

    std::unique_lock<std::mutex> lock{mutex};
    ASSERT_TRUE(cv.wait_for(lock, 5s, [&] { return ran; }))
        << "Scheduled work was not executed";
}

TEST_F(WaylandExecutorTest, runs_multiple_scheduled_work_in_order)
{
    int const num_work = 16;

    std::mutex mutex;
    std::condition_variable cv;
    std::vector<int> order;

    for (int i = 0; i < num_work; ++i)
    {
        executor->spawn(
            [&, i]
            {
                std::lock_guard<std::mutex> lock{mutex};
                order.push_back(i);
                cv.notify_one();
            });
    }

    std::unique_lock<std::mutex> lock{mutex};
    ASSERT_TRUE(cv.wait_for(lock, 5s, [&] { return order.size() == static_cast<size_t>(num_work); }))
        << "Not all scheduled work was executed";

    std::vector<int> expected(num_work);
    for (int i = 0; i < num_work; ++i)
        expected[i] = i;

    EXPECT_EQ(order, expected);
}

TEST_F(WaylandExecutorTest, runs_work_on_event_loop_thread_not_caller)
{
    std::mutex mutex;
    std::condition_variable cv;
    bool ran{false};
    std::thread::id work_thread;

    auto const caller_thread = std::this_thread::get_id();

    executor->spawn(
        [&]
        {
            std::lock_guard<std::mutex> lock{mutex};
            work_thread = std::this_thread::get_id();
            ran = true;
            cv.notify_one();
        });

    std::unique_lock<std::mutex> lock{mutex};
    ASSERT_TRUE(cv.wait_for(lock, 5s, [&] { return ran; }))
        << "Scheduled work was not executed";

    EXPECT_NE(work_thread, caller_thread);
}
