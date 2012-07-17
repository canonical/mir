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

#include "mock_buffer.h"

#include "mir/compositor/buffer_swapper_double.h"
#include "mir_test/multithread_harness.h"

#include <thread>

namespace mc = mir::compositor;
namespace mt = mir::testing;
namespace geom = mir::geometry;

template <typename S, typename T>
struct ThreadFixture {
    public:
        ThreadFixture(std::function<void(std::shared_ptr<S>, mt::Synchronizer<T>*, int)> a,
                      std::function<void(std::shared_ptr<S>, mt::Synchronizer<T>*, int)> b) 
        {
            std::chrono::milliseconds timeout(5000);
            geom::Width w {1024};
            geom::Height h {768};
            geom::Stride s {1024};
            mc::PixelFormat pf {mc::PixelFormat::rgba_8888};

            std::unique_ptr<mc::Buffer> buffer_a(new mc::MockBuffer(w, h, s, pf));
            std::unique_ptr<mc::Buffer> buffer_b(new mc::MockBuffer(w, h, s, pf));

            swapper = std::make_shared<mc::BufferSwapperDouble>(
                    std::move(buffer_a),
                    std::move(buffer_b));

            synchronizer = new mt::Synchronizer<mc::Buffer*>(2);

            t1 = std::thread(mt::manager_thread<mc::BufferSwapper, mc::Buffer*>,
                           a, swapper, 0, synchronizer, timeout);
            t2 = std::thread(mt::manager_thread<mc::BufferSwapper, mc::Buffer*>,
                           b, swapper, 1, synchronizer, timeout);
    
        };

        ~ThreadFixture()
        {
            t1.join();
            t2.join();
        }

        std::thread t1, t2;
        std::shared_ptr<mc::BufferSwapper> swapper;
        mt::Synchronizer<mc::Buffer*> *synchronizer;
};


void client_work_lockstep0(std::shared_ptr<mc::BufferSwapper> swapper,
                 mt::Synchronizer<mc::Buffer*>* synchronizer,
                 int tid )
{
    mc::Buffer* buf;
    for(;;)
    {

        buf = swapper->dequeue_free_buffer();
        synchronizer->set_thread_data(buf, tid);
        if (synchronizer->child_sync()) return;

        swapper->queue_finished_buffer();
        if (synchronizer->child_sync()) return;
    }
}

void server_work_lockstep0(std::shared_ptr<mc::BufferSwapper> swapper,
                 mt::Synchronizer<mc::Buffer*>* synchronizer,
                int tid )
{

    mc::Buffer* buf;
    for(;;)
    {
        buf = swapper->grab_last_posted();
        synchronizer->set_thread_data(buf, tid);
        if (synchronizer->child_sync()) return;

        swapper->ungrab();
        if (synchronizer->child_sync()) return;

    }

}

TEST(buffer_swapper_double_stress, simple_swaps0)
{
    const int num_iterations = 1000;
    ThreadFixture<mc::BufferSwapper, mc::Buffer*> fix(server_work_lockstep0, client_work_lockstep0);

    mc::Buffer* dequeued, *grabbed;
    for(int i=0; i< num_iterations; i++)
    {
        fix.synchronizer->control_wait();

        dequeued = fix.synchronizer->get_thread_data(0); 
        grabbed  = fix.synchronizer->get_thread_data(1); 
        EXPECT_NE(dequeued, grabbed);

        fix.synchronizer->control_activate();
 
        fix.synchronizer->control_wait();
        fix.synchronizer->control_activate();
    }

    fix.synchronizer->control_wait();
    fix.synchronizer->set_kill();
    fix.synchronizer->control_activate();

}


void client_work_timing0(std::shared_ptr<mc::BufferSwapper> swapper ,
                 mt::Synchronizer<mc::Buffer*>* synchronizer,
                 int tid )
{
    mc::Buffer* buf;
    for(;;)
    {
        buf = swapper->dequeue_free_buffer();
        swapper->queue_finished_buffer();
        synchronizer->set_thread_data(buf, tid);
        if(synchronizer->child_check_pause(tid)) break;
    }
}

void server_work_timing0(std::shared_ptr<mc::BufferSwapper> swapper,
                 mt::Synchronizer<mc::Buffer*>* synchronizer,
                int tid )
{
    mc::Buffer* buf;
    for (;;)
    {
        for (int j=0; j< 100; j++)
        {
            buf = swapper->grab_last_posted();
            swapper->ungrab();
        }

        synchronizer->set_thread_data(buf, tid);
        if(synchronizer->child_check_pause(tid)) break;
    }
}

TEST(buffer_swapper_double_timing, stress_swaps)
{
    ThreadFixture<mc::BufferSwapper, mc::Buffer*> fix(server_work_timing0, client_work_timing0);
    mc::Buffer* dequeued, *grabbed;
    const int num_it = 300;
    for(int i=0; i< num_it; i++)
    {
        fix.synchronizer->enforce_child_pause(1);
        fix.synchronizer->enforce_child_pause(0);
        fix.synchronizer->control_wait();

        dequeued = fix.synchronizer->get_thread_data(0); 
        grabbed  = fix.synchronizer->get_thread_data(1); 
        EXPECT_EQ(dequeued, grabbed);

        fix.synchronizer->control_activate(); 
    }

    /* kill all threads */
    fix.synchronizer->enforce_child_pause(1);
    fix.synchronizer->enforce_child_pause(0);
    fix.synchronizer->control_wait();
    fix.synchronizer->set_kill();
    fix.synchronizer->control_activate(); 

}
