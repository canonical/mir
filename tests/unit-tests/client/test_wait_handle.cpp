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

using namespace mir_toolkit;

TEST(WaitHandle, symmetric_synchronous)
{
    MirWaitHandle w;

    for (int i = 0; i < 100; i++)
    {
        w.result_received();
        w.wait_for_result();
    }

    EXPECT_TRUE(true);   // Failure would be hanging in the above loop
}

TEST(WaitHandle, asymmetric_synchronous)
{
    MirWaitHandle w;

    for (int i = 1; i < 100; i++)
    {
        for (int j = 1; j <= i; j++)
            w.result_received();

        w.wait_for_result();
    }

    EXPECT_TRUE(true);   // Failure would be hanging in the above loop
}

namespace
{
    int symmetric_result = 0;

    void symmetric_thread(MirWaitHandle *w, MirWaitHandle *q, int max)
    {
        for (int i = 1; i <= max; i++)
        {
            symmetric_result = i;
            w->result_received();
            q->wait_for_result();
        }
    }
}

TEST(WaitHandle, symmetric_asynchronous)
{
    const int max = 100;
    MirWaitHandle w, q;
    std::thread t(symmetric_thread, &w, &q, max);

    for (int i = 1; i <= max; i++)
    {
        w.wait_for_result();
        ASSERT_EQ(symmetric_result, i);
        q.result_received();
    }
    t.join();
}

namespace
{
    int asymmetric_result = 0;

    void asymmetric_thread(MirWaitHandle *w, MirWaitHandle *q, int max)
    {
        for (int i = 1; i <= max; i++)
        {
            asymmetric_result = i;
            for (int j = 1; j <= i; j++)
                w->result_received();
            q->wait_for_result();
        }
    }
}

TEST(WaitHandle, asymmetric_asynchronous)
{
    const int max = 100;
    MirWaitHandle w, q;
    std::thread t(asymmetric_thread, &w, &q, max);

    for (int i = 1; i <= max; i++)
    {
        for (int j = 1; j <= i; j++)
            w.expect_result();
        w.wait_for_result();
        ASSERT_EQ(asymmetric_result, i);
        q.result_received();
    }
    t.join();
}
