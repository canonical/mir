/*
 * Copyright © 2012 Canonical Ltd.
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
    WaitCondition() : woken_(false) {}

    void wait_for_at_most_seconds(int seconds)
    {
        std::unique_lock<std::mutex> ul(guard);
        condition.wait_for(ul, std::chrono::seconds(seconds),
                           [this] { return woken_; });
    }

    void wake_up_everyone()
    {
        std::unique_lock<std::mutex> ul(guard);
        woken_ = true;
        condition.notify_all();
    }

    bool woken()
    {
        std::unique_lock<std::mutex> ul(guard);
        return woken_;
    }

    std::mutex guard;
    std::condition_variable condition;
    bool woken_;
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
