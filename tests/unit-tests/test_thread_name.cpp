/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "mir/thread_name.h"
#include "mir/test/signal.h"

#include <thread>
#include <pthread.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mt = mir::test;

namespace
{

std::string name_of_thread(std::thread& t)
{
    static size_t const max_thread_name_size = 16;
    char thread_name[max_thread_name_size];

    pthread_getname_np(t.native_handle(), thread_name, sizeof thread_name);

    return {thread_name};
}

struct MirThreadName : ::testing::Test
{
    std::thread& start_thread_with_name(std::string const& name)
    {
        thread = std::thread{
            [this,name]
            {
                mir::set_thread_name(name);
                thread_name_set.raise();
                finish_thread.wait_for(std::chrono::seconds{5});
            }};

        thread_name_set.wait_for(std::chrono::seconds{5});

        return thread;
    }

    ~MirThreadName()
    {
        finish_thread.raise();
        if (thread.joinable())
            thread.join();
    }

private:
    std::thread thread;
    mt::Signal finish_thread;
    mt::Signal thread_name_set;
};

MATCHER_P(IsPrefixOf, value, "")
{
    return value.find(arg) == 0;
}

}

#ifdef HAVE_PTHREAD_GETNAME_NP
TEST_F(MirThreadName, sets_thread_name)
{
    using namespace ::testing;

    std::string const thread_name{"Mir:thread"};

    auto& thread = start_thread_with_name(thread_name);

    EXPECT_THAT(name_of_thread(thread), Eq(thread_name));
}

TEST_F(MirThreadName, sets_part_of_long_thread_name)
{
    using namespace ::testing;

    std::string const long_thread_name{"Mir:mylongthreadnameabcdefghijklmnopqrstuvwxyz"};

    auto& thread = start_thread_with_name(long_thread_name);

    EXPECT_THAT(name_of_thread(thread), IsPrefixOf(long_thread_name));
}
#endif
