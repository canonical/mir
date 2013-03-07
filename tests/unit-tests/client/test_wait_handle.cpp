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

    for (int i = 1; i <= 100; i++)
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

    void symmetric_thread(MirWaitHandle *in, MirWaitHandle *out, int max)
    {
        for (int i = 1; i <= max; i++)
        {
            in->wait_for_result();
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
        in.result_received();
        out.wait_for_result();
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
            in->wait_for_result();
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
        out.wait_for_result();
        ASSERT_EQ(asymmetric_result, i);
    }
    t.join();
}
