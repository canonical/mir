/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
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
 * Authored by: Thomas Voß <thomas.voss@canonical.com>
 */

#ifndef MIR_TEST_WAIT_CONDITION_H_
#define MIR_TEST_WAIT_CONDITION_H_

#include "mir/chrono/chrono.h"
#include "mir/thread/all.h"

namespace mir
{
struct WaitCondition
{
    void wait_for_at_most_seconds(int seconds)
    {
        std::unique_lock<std::mutex> ul(guard);
        condition.wait_for(ul, std::chrono::seconds(seconds));
    }

    void wake_up_everyone()
    {
        std::unique_lock<std::mutex> ul(guard);
        condition.notify_all();
    }

    std::mutex guard;
    std::condition_variable condition;
};
}

#endif // MIR_TEST_WAIT_CONDITION_H_
