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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "multithread_harness.h"

#include <thread>

namespace mt = mir::testing;

namespace
{
void test_func(mt::SynchronizerSpawned* synchronizer, int* data)
{
    *data = 1;
    synchronizer->child_enter_wait();
    *data = 2;
    synchronizer->child_enter_wait();
}
}

TEST(Synchronizer, thread_stop_start)
{
    int data = 0;

    mt::Synchronizer synchronizer;
    std::thread t1(test_func, &synchronizer, &data);

    synchronizer.ensure_child_is_waiting();
    EXPECT_EQ(data, 1);
    synchronizer.activate_waiting_child();

    synchronizer.ensure_child_is_waiting();
    EXPECT_EQ(data, 2);
    synchronizer.activate_waiting_child();

    t1.join();
}

namespace
{
void test_func_pause (mt::SynchronizerSpawned* synchronizer, int* data) {
    bool wait_request;
    for(;;)
    {
        wait_request = synchronizer->child_check_wait_request();
        *data = *data+1;

        if (wait_request)
        {
            if (synchronizer->child_enter_wait()) break;
        }
        std::this_thread::yield();
    }
}
}

TEST(Synchronizer, thread_pause_req)
{
    int data = 0, old_data = 0;

    mt::Synchronizer synchronizer;
    std::thread t1(test_func_pause, &synchronizer, &data);

    synchronizer.ensure_child_is_waiting();
    EXPECT_GT(data, old_data);
    old_data = data;
    synchronizer.activate_waiting_child();

    synchronizer.ensure_child_is_waiting();
    EXPECT_GT(data, old_data);
    synchronizer.activate_waiting_child();

    synchronizer.ensure_child_is_waiting();
    synchronizer.kill_thread();
    synchronizer.activate_waiting_child();

    t1.join();
}
