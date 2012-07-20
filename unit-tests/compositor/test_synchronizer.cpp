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

#include <vector>
#include <thread>

namespace mt = mir::testing;

void test_func (mt::SynchronizerSpawned* synchronizer, int* data) {
    *data = 1;
    synchronizer->child_enter_wait();
    *data = 2;
    synchronizer->child_enter_wait();
}

TEST(Synchronizer, thread_stop_start) {
    int data = 0;

    auto thread_start_time = std::chrono::system_clock::now();
    auto abs_timeout = thread_start_time + std::chrono::milliseconds(500);

    mt::Synchronizer synchronizer(abs_timeout);
    mt::ScopedThread scoped_thread(std::thread(test_func, &synchronizer, &data));

    synchronizer.ensure_child_is_waiting();
    EXPECT_EQ(data, 1);
    synchronizer.activate_waiting_child();

    synchronizer.ensure_child_is_waiting();
    EXPECT_EQ(data, 2);
    synchronizer.activate_waiting_child();
}

void test_func_pause (mt::SynchronizerSpawned* synchronizer, int* data) {
    for(;;)
    {
        *data = *data+1;
        if (synchronizer->child_check()) break;
    }
}

TEST(Synchronizer, thread_pause_req) {
    int data = 0;
    auto thread_start_time = std::chrono::system_clock::now();
    auto abs_timeout = thread_start_time + std::chrono::milliseconds(500);

    mt::Synchronizer synchronizer(abs_timeout);
    mt::ScopedThread scoped_thread(std::thread(test_func_pause, &synchronizer, &data));

    synchronizer.ensure_child_is_waiting();
    EXPECT_EQ(data, 1);
    synchronizer.activate_waiting_child();

    synchronizer.ensure_child_is_waiting();
    EXPECT_EQ(data, 2);
    synchronizer.activate_waiting_child();

    synchronizer.ensure_child_is_waiting();
    synchronizer.kill_thread();
    synchronizer.activate_waiting_child();
}
