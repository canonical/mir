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

#include "mir_test/multithread_harness.h"

#include <vector>
#include <thread>

namespace mt = mir::testing;


void test_func (mt::Synchronizer<int>* synchronizer) {
    synchronizer->child_sync();
}

TEST(Synchronizer, megathreaded) {
    const int num_threads = 1000;

    mt::Synchronizer<int> synchronizer(num_threads);

    std::vector<std::thread> t;
    for(int i=0; i<num_threads; i++)    
    {
        t.push_back(std::thread(test_func, &synchronizer));
    } 

    synchronizer.control_wait();
    synchronizer.control_activate();


    for(int i=0; i<num_threads; i++)
    {
        t[i].join();
    } 
}

void test_func_id (mt::Synchronizer<int>* synchronizer, int tid) {
    if (tid == 500) {
        while (true) 
        {
            if (synchronizer->child_check_pause(tid)) break;
        }
    } else {
        synchronizer->child_sync();
    }
}

TEST(Synchronizer, one_checker_thread) {
    const int num_threads = 1000;

    mt::Synchronizer<int> synchronizer(num_threads);

    std::vector<std::thread> t;
    for(int i=0; i<num_threads; i++)    
    {
        t.push_back(std::thread(test_func_id, &synchronizer, i));
    } 

    synchronizer.enforce_child_pause(500);
    synchronizer.control_wait();

    synchronizer.set_kill();
    synchronizer.control_activate();

    for(int i=0; i<num_threads; i++)
    {
        t[i].join();
    }
}
