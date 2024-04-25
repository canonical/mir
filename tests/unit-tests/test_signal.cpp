/*
 * Copyright © Canonical Ltd.
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

#include "mir/signal.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <thread>
#include <latch>

#include "mir/test/auto_unblock_thread.h"

namespace mt = mir::test;

TEST(Signal, waits_until_raised)
{
    mir::Signal signal;

    std::atomic<bool> event_hit{false};
    std::latch thread_spawned{2};

    std::thread waiter_thread{
        [&]()
        {
            thread_spawned.arrive_and_wait();
            EXPECT_FALSE(event_hit);
            signal.wait();
            EXPECT_TRUE(event_hit);
        }
    };

    thread_spawned.arrive_and_wait();
    std::this_thread::sleep_for(std::chrono::milliseconds{100});

    event_hit = true;
    signal.raise();

    waiter_thread.join();
}

TEST(Signal, racing_raises_release_at_least_one_waiter)
{
    mir::Signal signal;

    int const thread_count{10};
    std::latch threads_spawned{thread_count + 1};
    std::vector<mt::AutoJoinThread> racing_releasers;

    for(auto i = 0; i < thread_count; ++i)
    {
        racing_releasers.emplace_back(
            mt::AutoJoinThread{
                [&]()
                {
                    threads_spawned.arrive_and_wait();
                    signal.raise();
                }
        });
    }

    threads_spawned.arrive_and_wait();
    signal.wait();
}

TEST(Signal, pre_raised_signal_releases_immediately)
{
    mir::Signal signal;

    signal.raise();
    signal.wait();
}

TEST(Signal, release_from_wait_resets_signal)
{
    mir::Signal signal;

    std::chrono::milliseconds const delay{500};

    mt::AutoJoinThread releaser{
        [&]()
        {
            signal.raise();
            std::this_thread::sleep_for(delay * 2);
            signal.raise();            
        }
    };

    signal.wait();
    auto now = std::chrono::steady_clock::now();
    signal.wait();
    EXPECT_THAT(std::chrono::steady_clock::now(), testing::Gt(now + delay));
}
