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

/* todo: pass to worker threads */
#define NUM_ITERATIONS 100000
static mc::BufferSwapper *swapper;

void server_work()
{

    mc::Buffer* buf_tmp;
    for (int i=0; i<NUM_ITERATIONS; i++)
    {
        buf_tmp = swapper->dequeue_free_buffer();
        EXPECT_NE(nullptr, buf_tmp);
        swapper->queue_finished_buffer();
    }

}
void client_work()
{

    mc::Buffer* buf_tmp;
    for (int i=0; i<NUM_ITERATIONS; i++)
    {
        buf_tmp = swapper->grab_last_posted();
        EXPECT_NE(nullptr, buf_tmp);
        swapper->ungrab();
    }

}

void timeout_detection(std::function<void()> function, std::condition_variable *cv1, std::mutex* exec_lock )
{
    std::unique_lock<std::mutex> lk2(*exec_lock);
    /* once we acquire lock, we know that the parent thread's timer is running, so we can release it.
        if its not released, then a deadlock situation would prevent the wait_until() from ever waking */
    lk2.unlock();

    function();

    /* notify that we are done working */
    cv1->notify_one();
    return;
}

void client_thread(std::function<void()> function, std::mutex *exec_lock)
{
    std::condition_variable cv1;

    std::unique_lock<std::mutex> lk2(*exec_lock);
    std::thread(timeout_detection, function, &cv1, exec_lock).detach();

    /* set timeout as absolute time from this point */
    auto timeout_time = std::chrono::system_clock::now() + std::chrono::milliseconds(2000);
    if (std::cv_status::timeout == cv1.wait_until(lk2, timeout_time ))
    {
        FAIL();
    }

    return;
}

std::mutex exec_lock0;
std::mutex exec_lock1;
TEST(buffer_swapper_double_stress, simple_swaps)
{
    using namespace testing;

    geom::Width w {1024};
    geom::Height h {768};
    geom::Stride s {1024};
    mc::PixelFormat pf {mc::PixelFormat::rgba_8888};

    std::unique_ptr<mc::Buffer> buffer_a(new mc::MockBuffer(w, h, s, pf));
    std::unique_ptr<mc::Buffer> buffer_b(new mc::MockBuffer(w, h, s, pf));

    auto myswapper = std::make_shared<mc::BufferSwapperDouble>(
            std::move(buffer_a),
            std::move(buffer_b));

    swapper = myswapper.get();

    std::thread t1(client_thread, client_work, &exec_lock0);
    std::thread t2(client_thread, server_work, &exec_lock1);

    t1.join();
    t2.join();
}
