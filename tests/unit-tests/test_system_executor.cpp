/*
 * Copyright © 2022 Canonical Ltd.
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

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <future>

#include "mir/system_executor.h"
#include "mir/test/signal.h"

using namespace std::literals::chrono_literals;
using namespace testing;

namespace mt = mir::test;

TEST(SystemExecutor, executes_work)
{
    auto const done = std::make_shared<mt::Signal>();
    mir::system_executor.spawn([done]() { done->raise(); });

    EXPECT_TRUE(done->wait_for(60s));
}

TEST(SystemExecutor, can_execute_work_from_within_work_item)
{
    auto const done = std::make_shared<mt::Signal>();

    mir::system_executor.spawn(
        [done]()
        {
            mir::system_executor.spawn([done]() { done->raise(); });
        });

    EXPECT_TRUE(done->wait_for(60s));
}

TEST(SystemExecutor, work_executed_from_within_work_item_is_not_blocked_by_work_item_blocking)
{
    auto const done = std::make_shared<mt::Signal>();
    auto const waited_for_done = std::make_shared<mt::Signal>();

    mir::system_executor.spawn(
        [done, waited_for_done]()
        {
            mir::system_executor.spawn([done]() { done->raise(); });
            if (!waited_for_done->wait_for(60s))
            {
                FAIL() << "Spawned work failed to execute";
            }
        });

    EXPECT_TRUE(done->wait_for(60s));
    waited_for_done->raise();
}
