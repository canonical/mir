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

const int num_iterations = 1000;

void server_work(std::shared_ptr<mc::BufferSwapper> swapper ,
                 mt::Synchronizer<mc::Buffer*>* synchronizer,
                 int tid )
{
    mc::Buffer* buf;
    for (int i=0; i< num_iterations; i++)
    {
        buf = swapper->dequeue_free_buffer();

        swapper->queue_finished_buffer();

        synchronizer->set_thread_data(buf, tid);
        synchronizer->child_sync();
    }
}

void client_work(std::shared_ptr<mc::BufferSwapper> swapper,
                 mt::Synchronizer<mc::Buffer*>* synchronizer,
                int tid )
{

    mc::Buffer* buf;
    for (int i=0; i< num_iterations; i++)
    {
        buf = swapper->grab_last_posted();
        swapper->ungrab();

        synchronizer->set_thread_data(buf, tid);
        synchronizer->child_sync();
    }

}

TEST(buffer_swapper_double_stress, simple_swaps0)
{
    std::chrono::milliseconds timeout(5000);

    geom::Width w {1024};
    geom::Height h {768};
    geom::Stride s {1024};
    mc::PixelFormat pf {mc::PixelFormat::rgba_8888};

    std::unique_ptr<mc::Buffer> buffer_a(new mc::MockBuffer(w, h, s, pf));
    std::unique_ptr<mc::Buffer> buffer_b(new mc::MockBuffer(w, h, s, pf));

    auto swapper = std::make_shared<mc::BufferSwapperDouble>(
            std::move(buffer_a),
            std::move(buffer_b));

    /* use these condition variables to poke and control the two threads */
    mt::Synchronizer<mc::Buffer*> synchronizer(2);

    std::thread t1(mt::manager_thread<mc::BufferSwapper, mc::Buffer*>,
                   client_work, swapper, 0, &synchronizer, timeout);
    std::thread t2(mt::manager_thread<mc::BufferSwapper, mc::Buffer*>,
                   server_work, swapper, 1, &synchronizer, timeout);

    mc::Buffer* dequeued, *grabbed;
    for(int i=0; i< num_iterations; i++)
    {
        synchronizer.control_wait();

        dequeued = synchronizer.get_thread_data(0); 
        grabbed  = synchronizer.get_thread_data(1); 
        ASSERT_NE(dequeued, grabbed);

        synchronizer.control_activate(); 
    }

    t1.join();
    t2.join();
}
