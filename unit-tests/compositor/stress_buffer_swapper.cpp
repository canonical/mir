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

#include <mir/compositor/buffer_swapper_double.h>
#include <thread>

namespace mc = mir::compositor;
namespace geom = mir::geometry;

void client_work(std::shared_ptr<mc::BufferSwapper> swapper )
{
    mc::Buffer* buf_tmp;

    const int num_iterations = 10000; 

    for (int i=0; i< num_iterations; i++)
    {
        buf_tmp = swapper->dequeue_free_buffer();
        EXPECT_TRUE(buf_tmp);
        swapper->queue_finished_buffer();
    }
}

void server_work(std::shared_ptr<mc::BufferSwapper> swapper )
{
    const int num_iterations = 10000; 

    mc::Buffer* buf_tmp;
    for (int i=0; i< num_iterations; i++)
    {
        buf_tmp = swapper->grab_last_posted();
        EXPECT_TRUE(buf_tmp);
        swapper->ungrab();
    }

}

void timeout_detection(std::function<void(std::shared_ptr<mc::BufferSwapper>)> function,
                       std::condition_variable *cv1, std::mutex* exec_lock,
                       std::shared_ptr<mc::BufferSwapper> swapper )
{
    std::unique_lock<std::mutex> lk2(*exec_lock);
    /* once we acquire lock, we know that the parent thread's timer is running, so we can release it.
        if its not released, then a deadlock situation would prevent the wait_until() from ever waking */
    lk2.unlock();

    function(swapper);

    /* notify that we are done working */
    cv1->notify_one();
    return;
}


/* todo: std::function<> argument is not very generic... */
void client_thread(std::function<void(std::shared_ptr<mc::BufferSwapper>)> function,
                   std::mutex *exec_lock, std::shared_ptr<mc::BufferSwapper> swapper,
                   std::chrono::milliseconds timeout)
{
    std::condition_variable cv1;

    std::unique_lock<std::mutex> lk2(*exec_lock);
    std::thread(timeout_detection, function, &cv1, exec_lock, swapper).detach();

    /* set timeout as absolute time from this point */
    auto timeout_time = std::chrono::system_clock::now() + timeout;

    /* once the cv starts waiting, it releases the lock and triggers the timeout-protected 
       thread that it can start the execution of the timeout function. this way the tiemout
       thread cannot send a cv notify_one before we are actually waiting (as with a very short
       target function */
    if (cv1.wait_until(lk2, timeout_time ))
    {
        FAIL();
    }

    return;
}

TEST(buffer_swapper_double_stress, simple_swaps)
{
    std::mutex exec_lock0;
    std::mutex exec_lock1;
    std::chrono::milliseconds timeout(400);

    geom::Width w {1024};
    geom::Height h {768};
    geom::Stride s {1024};
    mc::PixelFormat pf {mc::PixelFormat::rgba_8888};

    std::unique_ptr<mc::Buffer> buffer_a(new mc::MockBuffer(w, h, s, pf));
    std::unique_ptr<mc::Buffer> buffer_b(new mc::MockBuffer(w, h, s, pf));

    auto swapper = std::make_shared<mc::BufferSwapperDouble>(
            std::move(buffer_a),
            std::move(buffer_b));

    std::thread t1(client_thread, client_work, &exec_lock0, swapper, timeout);
    std::thread t2(client_thread, server_work, &exec_lock1, swapper, timeout);
    t1.join();
    t2.join();
}
