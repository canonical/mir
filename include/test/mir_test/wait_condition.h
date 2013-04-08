/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Thomas Voß <thomas.voss@canonical.com>
 */

#ifndef MIR_TEST_WAIT_CONDITION_H_
#define MIR_TEST_WAIT_CONDITION_H_

#include <gmock/gmock.h>

#include <chrono>
#include <mutex>
#include <condition_variable>

namespace mir
{
namespace test
{
struct WaitCondition
{
    WaitCondition() : woken(false) {}

    void wait_for_at_most_seconds(int seconds)
    {
        std::unique_lock<std::mutex> ul(guard);
        if (!woken) condition.wait_for(ul, std::chrono::seconds(seconds));
    }

    void wake_up_everyone()
    {
        std::unique_lock<std::mutex> ul(guard);
        woken = true;
        condition.notify_all();
    }

    std::mutex guard;
    std::condition_variable condition;
    bool woken;
};

ACTION_P(ReturnFalseAndWakeUp, wait_condition)
{
    wait_condition->wake_up_everyone();
    return false;
}
ACTION_P(WakeUp, wait_condition)
{
    wait_condition->wake_up_everyone();
}
}
}

#endif // MIR_TEST_WAIT_CONDITION_H_
