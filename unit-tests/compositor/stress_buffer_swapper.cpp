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
#include "multithread_harness.h"

#include <thread>
#include <memory>
#include <functional>

namespace mc = mir::compositor;
namespace mt = mir::testing;
namespace geom = mir::geometry;

struct ThreadFixture {
    public:
        ThreadFixture(
            std::function<void( mt::SynchronizerSpawned*,
                            std::shared_ptr<mc::BufferSwapper>,
                            mc::Buffer** )> a,
            std::function<void( mt::SynchronizerSpawned*,
                            std::shared_ptr<mc::BufferSwapper>,
                            mc::Buffer** )> b)
        {
            geom::Width w {1024};
            geom::Height h {768};
            geom::Stride s {1024};
            mc::PixelFormat pf {mc::PixelFormat::rgba_8888};

            std::unique_ptr<mc::Buffer> buffer_a(new mc::MockBuffer(w, h, s, pf));
            std::unique_ptr<mc::Buffer> buffer_b(new mc::MockBuffer(w, h, s, pf));
            auto swapper = std::make_shared<mc::BufferSwapperDouble>(
                    std::move(buffer_a),
                    std::move(buffer_b));

            auto thread_start_time = std::chrono::system_clock::now();
            auto abs_timeout = thread_start_time + std::chrono::milliseconds(1000);
            auto sync1 = new mt::Synchronizer(abs_timeout);
            auto sync2 = new mt::Synchronizer(abs_timeout);

            thread1 = new mt::ScopedThread(std::thread(a, sync1, swapper, &buffer1));
            thread2 = new mt::ScopedThread(std::thread(b, sync2, swapper, &buffer2));

            controller1 = sync1;
            controller2 = sync2;
        };

        ~ThreadFixture()
        {
            controller2->ensure_child_is_waiting();
            controller2->kill_thread();
            controller2->activate_waiting_child();

            controller1->ensure_child_is_waiting();
            controller1->kill_thread();
            controller1->activate_waiting_child();

            delete controller1;
            delete controller2;
            delete thread1;
            delete thread2;
        }

        mt::ScopedThread* thread1;
        mt::ScopedThread* thread2;
        mt::SynchronizerController *controller1;
        mt::SynchronizerController *controller2;
        mc::Buffer *buffer1;
        mc::Buffer *buffer2;
};

void client_request_loop( mt::SynchronizerSpawned* synchronizer,
                            std::shared_ptr<mc::BufferSwapper> swapper,
                            mc::Buffer** buf )
{
    for(;;)
    {
        *buf = swapper->dequeue_free_buffer();
        if (synchronizer->child_enter_wait()) return;

        swapper->queue_finished_buffer(*buf);
        if (synchronizer->child_enter_wait()) return;
    }
}

void compositor_grab_loop( mt::SynchronizerSpawned* synchronizer,
                            std::shared_ptr<mc::BufferSwapper> swapper,
                            mc::Buffer** buf )
{
    for(;;)
    {
        *buf = swapper->grab_last_posted();
        if (synchronizer->child_enter_wait()) return;

        swapper->ungrab(*buf);
        if (synchronizer->child_enter_wait()) return;

    }

}
/* test that the compositor and the client are never in ownership of the same
   buffer */
TEST(buffer_swapper_double_stress, distinct_buffers_in_client_and_compositor)
{
    const int num_iterations = 1000;
    ThreadFixture fix(compositor_grab_loop, client_request_loop);
    for(int i=0; i<  num_iterations; i++)
    {
        fix.controller1->ensure_child_is_waiting();
        fix.controller2->ensure_child_is_waiting();

        EXPECT_NE(fix.buffer1, fix.buffer2);

        fix.controller1->activate_waiting_child();
        fix.controller2->activate_waiting_child();

    }

}
/* test that we never get an invalid buffer */
TEST(buffer_swapper_double_stress, ensure_valid_buffers)
{
    const int num_iterations = 1000;
    ThreadFixture fix(compositor_grab_loop, client_request_loop);

    for(int i=0; i< num_iterations; i++)
    {
        fix.controller1->ensure_child_is_waiting();
        fix.controller2->ensure_child_is_waiting();

        if (i > 1)
        {
            ASSERT_NE(fix.buffer1, nullptr);
            ASSERT_NE(fix.buffer2, nullptr);
        }

        fix.controller1->activate_waiting_child();
        fix.controller2->activate_waiting_child();

    }
}

void client_will_wait( mt::SynchronizerSpawned* synchronizer,
                            std::shared_ptr<mc::BufferSwapper> swapper,
                            mc::Buffer** buf )
{
    *buf = swapper->dequeue_free_buffer();
    swapper->queue_finished_buffer(*buf);

    synchronizer->child_enter_wait();

    *buf = swapper->dequeue_free_buffer();

    synchronizer->child_enter_wait();
}

void compositor_grab( mt::SynchronizerSpawned* synchronizer,
                            std::shared_ptr<mc::BufferSwapper> swapper,
                            mc::Buffer** buf )
{
    synchronizer->child_enter_wait();

    *buf = swapper->grab_last_posted();
    swapper->ungrab(*buf);

    synchronizer->child_enter_wait();
    synchronizer->child_enter_wait();
}

/* test a wait situation */
TEST(buffer_swapper_double_stress, test_wait)
{
    ThreadFixture fix(compositor_grab, client_will_wait);

    mc::Buffer* first_dequeued;

    fix.controller1->ensure_child_is_waiting();
    fix.controller2->ensure_child_is_waiting();

    first_dequeued = fix.buffer2;

    fix.controller2->activate_waiting_child();

    /* activate grab */
    fix.controller1->activate_waiting_child();
    fix.controller1->ensure_child_is_waiting();
    fix.controller1->activate_waiting_child();

    EXPECT_EQ(first_dequeued, fix.buffer1);

}










void client_request_loop_with_wait( mt::SynchronizerSpawned* synchronizer,
                            std::shared_ptr<mc::BufferSwapper> swapper,
                            mc::Buffer** buf )
{
    bool wait_request = false;
    for(;;)
    {
        wait_request = synchronizer->child_check_wait_request();

        *buf = swapper->dequeue_free_buffer();
        swapper->queue_finished_buffer(*buf);

        if (wait_request)
            if (synchronizer->child_enter_wait()) return;
    }
}


void compositor_grab_loop_with_wait( mt::SynchronizerSpawned* synchronizer,
                            std::shared_ptr<mc::BufferSwapper> swapper,
                            mc::Buffer** buf )
{
    bool wait_request = false;
    for(;;)
    {
        wait_request = synchronizer->child_check_wait_request();

        *buf = swapper->grab_last_posted();

        swapper->ungrab(*buf);

        if (wait_request)
            if (synchronizer->child_enter_wait()) return;

    }

}

TEST(buffer_swapper_double_stress, test_last_posted)
{
    const int num_iterations = 1000;
    ThreadFixture fix(compositor_grab_loop_with_wait, client_request_loop_with_wait);
    for(int i=0; i<  num_iterations; i++)
    {
        fix.controller2->ensure_child_is_waiting();
        fix.controller1->ensure_child_is_waiting();

        EXPECT_EQ(fix.buffer1, fix.buffer2);

        fix.controller1->activate_waiting_child();
        fix.controller2->activate_waiting_child();

    }

}

void client_request_loop_stress_wait( mt::SynchronizerSpawned* synchronizer,
                            std::shared_ptr<mc::BufferSwapper> swapper,
                            mc::Buffer** buf )
{
    bool wait_request = false;
    for(;;)
    {
        wait_request = synchronizer->child_check_wait_request();

        *buf = swapper->dequeue_free_buffer();
        swapper->queue_finished_buffer(*buf);

        /* two dequeues in quick succession like this will wait very often
           hence the 'stress' */
        *buf = swapper->dequeue_free_buffer();
        swapper->queue_finished_buffer(*buf);

        if (wait_request)
            if (synchronizer->child_enter_wait()) return;
    }
}

TEST(buffer_swapper_double_stress, test_last_posted_stress_client_wait)
{
    const int num_iterations = 1000;
    ThreadFixture fix(compositor_grab_loop_with_wait, client_request_loop_stress_wait);
    for(int i=0; i<  num_iterations; i++)
    {
        fix.controller2->ensure_child_is_waiting();
        fix.controller1->ensure_child_is_waiting();

        EXPECT_EQ(fix.buffer1, fix.buffer2);

        fix.controller1->activate_waiting_child();
        fix.controller2->activate_waiting_child();

    }

}
