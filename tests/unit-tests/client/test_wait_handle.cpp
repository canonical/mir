/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Daniel van Vugt <daniel.van.vugt@canonical.com>
 */

#include <thread>
#include <gtest/gtest.h>
#include "src/client/mir_wait_handle.h"

namespace
{
int const arbitrary_number_of_events = 100;
}

TEST(WaitHandle, symmetric_synchronous)
{
    MirWaitHandle w;
    for (int i = 0; i != arbitrary_number_of_events; ++i)
        w.expect_result();

    for (int i = 0; i < arbitrary_number_of_events; i++)
    {
        w.result_received();
    }

    w.wait_for_all();

    EXPECT_TRUE(true);   // Failure would be hanging in the above loop
}

TEST(WaitHandle, asymmetric_synchronous)
{
    MirWaitHandle w;

    for (int i = 1; i <= arbitrary_number_of_events; i++)
    {
        for (int j = 0; j != i; ++j)
            w.expect_result();

        for (int j = 1; j <= i; j++)
            w.result_received();

        w.wait_for_all();
    }

    EXPECT_TRUE(true);   // Failure would be hanging in the above loop
}

namespace
{
    int symmetric_result = 0;

    void symmetric_thread(MirWaitHandle *in, MirWaitHandle *out, int max)
    {
        for (int i = 1; i <= max; i++)
        {
            in->expect_result();
            in->wait_for_all();
            symmetric_result = i;
            out->result_received();
        }
    }
}

TEST(WaitHandle, symmetric_asynchronous)
{
    const int max = 100;
    MirWaitHandle in, out;
    std::thread t(symmetric_thread, &in, &out, max);

    for (int i = 1; i <= max; i++)
    {
        out.expect_result();
        in.result_received();
        out.wait_for_all();
        ASSERT_EQ(symmetric_result, i);
    }
    t.join();
}

namespace
{
    int asymmetric_result = 0;

    void asymmetric_thread(MirWaitHandle *in, MirWaitHandle *out, int max)
    {
        for (int i = 1; i <= max; i++)
        {
            in->expect_result();
            in->wait_for_all();
            asymmetric_result = i;
            for (int j = 1; j <= i; j++)
                out->result_received();
        }
    }
}

TEST(WaitHandle, asymmetric_asynchronous)
{
    const int max = 100;
    MirWaitHandle in, out;
    std::thread t(asymmetric_thread, &in, &out, max);

    for (int i = 1; i <= max; i++)
    {
        in.result_received();
        for (int j = 1; j <= i; j++)
            out.expect_result();
        out.wait_for_all();
        ASSERT_EQ(asymmetric_result, i);
    }
    t.join();
}

namespace
{
    void crowded_thread(MirWaitHandle *w, int max)
    {
        for (int i = 0; i < max; i++)
        {
            /*
             * This doesn't work with wait_for_all, because waiters will
             * race and the winner would gobble up all the results, leaving
             * the other waiters hanging forever. So we have wait_for_one()
             * to allow many threads to safely wait on the same handle and
             * guarantee that they will all get the number of results they
             * expect.
             */
            w->wait_for_one();
        }
    }
}

TEST(WaitHandle, many_waiters)
{
    const int max = 100;
    MirWaitHandle w;
    std::thread a(crowded_thread, &w, max);
    std::thread b(crowded_thread, &w, max);
    std::thread c(crowded_thread, &w, max);

    for (int n = 0; n < max * 3; n++)
        w.result_received();

    a.join();
    b.join();
    c.join();
}

