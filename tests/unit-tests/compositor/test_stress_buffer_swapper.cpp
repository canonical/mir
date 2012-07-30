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
#include "multithread_harness.h"

#include "mir/compositor/buffer_swapper_double.h"
#include "mir/thread/all.h"

namespace mc = mir::compositor;
namespace mt = mir::testing;
namespace geom = mir::geometry;

struct ThreadFixture {
    public:
        ThreadFixture(
            std::function<void( std::shared_ptr<mt::SynchronizerSpawned>,
                            std::shared_ptr<mc::BufferSwapper>,
                            mc::Buffer** )> a,
            std::function<void( std::shared_ptr<mt::SynchronizerSpawned>,
                            std::shared_ptr<mc::BufferSwapper>,
                            mc::Buffer** )> b)
        {
            geom::Width w {1024};
            geom::Height h {768};
            geom::Stride s {1024};
            mc::PixelFormat pf {mc::PixelFormat::rgba_8888};
            std::shared_ptr<mc::Buffer> buffer_a(new mc::MockBuffer(w, h, s, pf));
            std::shared_ptr<mc::Buffer> buffer_b(new mc::MockBuffer(w, h, s, pf));

            auto swapper = std::make_shared<mc::BufferSwapperDouble>(
                    std::move(buffer_a),
                    std::move(buffer_b));

            auto sync1 = std::make_shared<mt::Synchronizer>();
            auto sync2 = std::make_shared<mt::Synchronizer>();
            compositor_controller = sync1;
            client_controller = sync2;

            thread1 = std::thread(a, sync1, swapper, &compositor_buffer);
            thread2 = std::thread(b, sync2, swapper, &client_buffer);
#ifndef MIR_USING_BOOST_THREADS
            sleep_duration = std::chrono::microseconds( 50 );
#else
            sleep_duration = ::boost::posix_time::microseconds( 50 );
#endif
        };

        ~ThreadFixture()
        {
            client_controller->ensure_child_is_waiting();
            client_controller->kill_thread();
            client_controller->activate_waiting_child();

            compositor_controller->ensure_child_is_waiting();
            compositor_controller->kill_thread();
            compositor_controller->activate_waiting_child();

            thread2.join();
            thread1.join();
        };

        std::shared_ptr<mt::SynchronizerController> compositor_controller;
        std::shared_ptr<mt::SynchronizerController> client_controller;

        mc::Buffer *compositor_buffer;
        mc::Buffer *client_buffer;

#ifndef MIR_USING_BOOST_THREADS
        std::chrono::microseconds sleep_duration;
#else
        ::boost::posix_time::time_duration sleep_duration;
#endif

    private:
        /* thread objects must exist over lifetime of test */ 
        std::thread thread1;
        std::thread thread2;
};


/* pause the main loop in between iterations */
/* todo (kdub): resolve how to sleep a thread using gcc 4.4's std */
#ifndef MIR_USING_BOOST_THREADS
void main_test_loop_pause(std::chrono::microseconds duration) {
        std::this_thread::sleep_for( duration );
}
#else
void main_test_loop_pause(boost::posix_time::time_duration duration) {
        ::boost::this_thread::sleep(duration);
}
#endif

void client_request_loop( std::shared_ptr<mt::SynchronizerSpawned> synchronizer,
                            std::shared_ptr<mc::BufferSwapper> swapper,
                            mc::Buffer** buf )
{
    std::shared_ptr<mc::Buffer> buf_tmp;
    for(;;)
    {
        buf_tmp = swapper->client_acquire();
        *buf = buf_tmp.get();
        if (synchronizer->child_enter_wait()) return;

        swapper->client_release(buf_tmp);
        if (synchronizer->child_enter_wait()) return;
    }
}

void compositor_grab_loop( std::shared_ptr<mt::SynchronizerSpawned> synchronizer,
                            std::shared_ptr<mc::BufferSwapper> swapper,
                            mc::Buffer** buf )
{
    std::shared_ptr<mc::Buffer> buf_tmp;
    for(;;)
    {
        buf_tmp = swapper->compositor_acquire();
        *buf = buf_tmp.get();
        if (synchronizer->child_enter_wait()) return;

        swapper->compositor_release(buf_tmp);
        if (synchronizer->child_enter_wait()) return;

    }

}

/* test that the compositor and the client are never in ownership of the same
   buffer */
TEST(buffer_swapper_double_stress, distinct_buffers_in_client_and_compositor)
{
    const int num_iterations = 500;
    ThreadFixture fix(compositor_grab_loop, client_request_loop);
    for(int i=0; i<  num_iterations; i++)
    {
        fix.compositor_controller->ensure_child_is_waiting();
        fix.client_controller->ensure_child_is_waiting();

        EXPECT_NE(fix.compositor_buffer, fix.client_buffer);

        fix.compositor_controller->activate_waiting_child();
        fix.client_controller->activate_waiting_child();

        main_test_loop_pause(fix.sleep_duration);
    }

}

/* test that we never get an invalid buffer */
TEST(buffer_swapper_double_stress, ensure_valid_buffers)
{
    const int num_iterations = 500;
    ThreadFixture fix(compositor_grab_loop, client_request_loop);

    for(int i=0; i< num_iterations; i++)
    {
        fix.compositor_controller->ensure_child_is_waiting();
        fix.client_controller->ensure_child_is_waiting();

        if (i > 1)
        {
            EXPECT_TRUE((fix.compositor_buffer != NULL));
            EXPECT_TRUE((fix.client_buffer != NULL));
        }

        fix.compositor_controller->activate_waiting_child();
        fix.client_controller->activate_waiting_child();

        main_test_loop_pause(fix.sleep_duration);

    }
}


void client_will_wait( std::shared_ptr<mt::SynchronizerSpawned> synchronizer,
                            std::shared_ptr<mc::BufferSwapper> swapper,
                            mc::Buffer** buf )
{
    std::shared_ptr<mc::Buffer> buf_tmp;

    buf_tmp = swapper->client_acquire();
    *buf = buf_tmp.get();
    swapper->client_release(buf_tmp);

    synchronizer->child_enter_wait();

    buf_tmp = swapper->client_acquire();
    *buf = buf_tmp.get();

    synchronizer->child_enter_wait();
}

void compositor_grab( std::shared_ptr<mt::SynchronizerSpawned> synchronizer,
                            std::shared_ptr<mc::BufferSwapper> swapper,
                            mc::Buffer** buf )
{
    std::shared_ptr<mc::Buffer> buf_tmp;

    synchronizer->child_enter_wait();

    buf_tmp = swapper->compositor_acquire();
    *buf = buf_tmp.get();
    swapper->compositor_release(buf_tmp);

    synchronizer->child_enter_wait();
    synchronizer->child_enter_wait();
}

/* test a simple wait situation due to no available buffers */
TEST(buffer_swapper_double_stress, test_wait)
{
    ThreadFixture fix(compositor_grab, client_will_wait);

    mc::Buffer* first_dequeued;

    fix.compositor_controller->ensure_child_is_waiting();
    fix.client_controller->ensure_child_is_waiting();

    first_dequeued = fix.client_buffer;

    fix.client_controller->activate_waiting_child();

    /* activate grab */
    fix.compositor_controller->activate_waiting_child();
    fix.compositor_controller->ensure_child_is_waiting();
    fix.compositor_controller->activate_waiting_child();

    EXPECT_EQ(first_dequeued, fix.compositor_buffer);

}

void client_request_loop_with_wait( std::shared_ptr<mt::SynchronizerSpawned> synchronizer,
                            std::shared_ptr<mc::BufferSwapper> swapper,
                            mc::Buffer** buf )
{
    std::shared_ptr<mc::Buffer> buf_tmp;
    bool wait_request = false;
    for(;;)
    {
        wait_request = synchronizer->child_check_wait_request();

        buf_tmp = swapper->client_acquire();
        *buf = buf_tmp.get();
        swapper->client_release(buf_tmp);

        if (wait_request)
            if (synchronizer->child_enter_wait()) return;
    }
}

void client_request_loop_stress_wait( std::shared_ptr<mt::SynchronizerSpawned> synchronizer,
                            std::shared_ptr<mc::BufferSwapper> swapper,
                            mc::Buffer** buf )
{
    std::shared_ptr<mc::Buffer> buf_tmp;
    bool wait_request = false;
    for(;;)
    {
        wait_request = synchronizer->child_check_wait_request();

        buf_tmp = swapper->client_acquire();
        *buf = buf_tmp.get();
        swapper->client_release(buf_tmp);

        /* two dequeues in quick succession like this will wait very often
           hence the 'stress' */
        buf_tmp = swapper->client_acquire();
        *buf = buf_tmp.get();
        swapper->client_release(buf_tmp);

        if (wait_request)
            if (synchronizer->child_enter_wait()) return;
    }
}

void compositor_grab_loop_with_wait( std::shared_ptr<mt::SynchronizerSpawned> synchronizer,
                            std::shared_ptr<mc::BufferSwapper> swapper,
                            mc::Buffer** buf )
{
    std::shared_ptr<mc::Buffer> buf_tmp;
    bool wait_request = false;
    for(;;)
    {
        wait_request = synchronizer->child_check_wait_request();

        buf_tmp = swapper->compositor_acquire();
        *buf = buf_tmp.get();

        swapper->compositor_release(buf_tmp);

        if (wait_request)
            if (synchronizer->child_enter_wait()) return;

#ifndef MIR_USING_BOOST_THREADS
        std::this_thread::yield();
#else
        boost::this_thread::yield();
#endif 
    }

}

/* test normal situation, with moderate amount of waits */
TEST(buffer_swapper_double_stress, test_last_posted)
{
    const int num_iterations = 500;
    ThreadFixture fix(compositor_grab_loop_with_wait, client_request_loop_with_wait);
    for(int i=0; i<  num_iterations; i++)
    {
        fix.client_controller->ensure_child_is_waiting();
        fix.compositor_controller->ensure_child_is_waiting();

        EXPECT_EQ(fix.compositor_buffer, fix.client_buffer);

        fix.compositor_controller->activate_waiting_child();
        fix.client_controller->activate_waiting_child();

        main_test_loop_pause(fix.sleep_duration);
    }

}

/* test situation where we'd wait on resoures more than normal, with moderate amount of waits */
TEST(buffer_swapper_double_stress, test_last_posted_stress_client_wait)
{
    const int num_iterations = 500;
    ThreadFixture fix(compositor_grab_loop_with_wait, client_request_loop_stress_wait);
    for(int i=0; i<  num_iterations; i++)
    {
        fix.client_controller->ensure_child_is_waiting();
        fix.compositor_controller->ensure_child_is_waiting();

        EXPECT_EQ(fix.compositor_buffer, fix.client_buffer);

        fix.compositor_controller->activate_waiting_child();
        fix.client_controller->activate_waiting_child();

        main_test_loop_pause(fix.sleep_duration);
    }
}
