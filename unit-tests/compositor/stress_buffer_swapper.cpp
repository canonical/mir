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
#include <iostream>

namespace mc = mir::compositor;
namespace mt = mir::testing;
namespace geom = mir::geometry;

void server_work(std::shared_ptr<mc::BufferSwapper> swapper,
                 mt::Synchronizer<mc::Buffer*>*  )
{
    std::mutex cv_mutex;
    std::unique_lock<std::mutex> lk(cv_mutex);

    mc::Buffer* buf_tmp_a, *buf_tmp_b;

    const int num_iterations = 10000;

    for (int i=0; i< num_iterations; i++)
    {
        buf_tmp_a = swapper->dequeue_free_buffer();
        EXPECT_NE(nullptr, buf_tmp_a);
        swapper->queue_finished_buffer();

        buf_tmp_b = swapper->dequeue_free_buffer();
        EXPECT_NE(nullptr, buf_tmp_b);
        swapper->queue_finished_buffer();

        EXPECT_NE(buf_tmp_a, buf_tmp_b);
    }

}

void client_work(std::shared_ptr<mc::BufferSwapper> swapper,
                 mt::Synchronizer<mc::Buffer*>*  )
{
    std::mutex cv_mutex;
    std::unique_lock<std::mutex> lk(cv_mutex);


    mc::Buffer* buf_tmp;

    const int num_iterations = 10000;
    for (int i=0; i< num_iterations; i++)
    {
        buf_tmp = swapper->grab_last_posted();
        EXPECT_NE(nullptr, buf_tmp);
        swapper->ungrab();
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
    mt::Synchronizer<mc::Buffer*> synchronizer;
    std::thread t1(mt::manager_thread<mc::BufferSwapper, mc::Buffer*>,
                   client_work, swapper, &synchronizer, timeout);
    std::thread t2(mt::manager_thread<mc::BufferSwapper, mc::Buffer*>,
                   server_work, swapper, &synchronizer, timeout);
    /* wait for sync to notify us that the thread has started. */


    /* wait for a wake up. if the thread has been killed by the timeout, then
       we know to abort. because we set a timeout on the thread that we are testing,
       we will be woken up with a kill signal regardless of success or failure */
    std::mutex tmp;
    std::unique_lock<std::mutex> lk(tmp);


    /* start multithreaded test expectations */
    while (! synchronizer.sync() )
    {
        /* system is in stable state here */ 
        /* check expectations */


        /* stimulate the system to run */ 
        /* allow threads to come into stable state */
    }

    t1.join();
    t2.join();
}
