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

#include "mir_test/mock_buffer.h"
#include "multithread_harness.h"

#include "mir/chrono/chrono.h"
#include "mir/compositor/buffer_swapper_multi.h"
#include "mir/thread/all.h"

namespace mc = mir::compositor;
namespace mt = mir::testing;
namespace geom = mir::geometry;

namespace mir
{
struct BufferSwapperStress : public ::testing::Test
{
public:
    BufferSwapperStress()
     : sleep_duration(50)
    {};
    void SetUp()
    {
        geom::Size size{geom::Width{1024}, geom::Height{768}};
        geom::Stride s {1024};
        geom::PixelFormat pf {geom::PixelFormat::rgba_8888};

        double_swapper = std::make_shared<mc::BufferSwapperMulti>(
                    std::move(std::unique_ptr<mc::Buffer>(new mc::MockBuffer(size, s, pf))),
                    std::move(std::unique_ptr<mc::Buffer>(new mc::MockBuffer(size, s, pf))));
        triple_swapper = std::make_shared<mc::BufferSwapperMulti>(
                    std::move(std::unique_ptr<mc::Buffer>(new mc::MockBuffer(size, s, pf))),
                    std::move(std::unique_ptr<mc::Buffer>(new mc::MockBuffer(size, s, pf))),
                    std::move(std::unique_ptr<mc::Buffer>(new mc::MockBuffer(size, s, pf))));

        num_iterations = 500;
    }

    void terminate_child_thread(mt::Synchronizer& controller)
    {
        controller.ensure_child_is_waiting();
        controller.kill_thread();
        controller.activate_waiting_child();
    }

    std::shared_ptr<mc::BufferSwapper> double_swapper;
    std::shared_ptr<mc::BufferSwapper> triple_swapper;

    mt::Synchronizer compositor_controller;
    mt::Synchronizer client_controller;

    mc::Buffer *compositor_buffer;
    mc::Buffer *client_buffer;

    std::chrono::microseconds const sleep_duration;
    int num_iterations;

    std::thread thread1;
    std::thread thread2;

    void test_distinct_buffers(const std::shared_ptr<mc::BufferSwapper>& swapper);
    void test_valid_buffers(const std::shared_ptr<mc::BufferSwapper>& swapper);
    void test_wait_situation(std::vector<mc::Buffer*>& compositor_output_buffers,
                             std::vector<mc::Buffer*>& client_output_buffers,
                             const std::shared_ptr<mc::BufferSwapper>& swapper,
                             unsigned int number_of_client_requests_to_make);
    void test_last_posted(const std::shared_ptr<mc::BufferSwapper>& swapper);
};

void main_test_loop_pause(std::chrono::microseconds duration) {
    std::this_thread::sleep_for(duration);
}

void client_request_loop( mt::SynchronizerSpawned& synchronizer,
                            const std::shared_ptr<mc::BufferSwapper>& swapper,
                            mc::Buffer** buf )
{
    for(;;)
    {
        *buf = swapper->client_acquire();
        if (synchronizer.child_enter_wait()) return;

        swapper->client_release(*buf);
        if (synchronizer.child_enter_wait()) return;

        std::this_thread::yield();
    }
}

void compositor_grab_loop( mt::SynchronizerSpawned& synchronizer,
                           const std::shared_ptr<mc::BufferSwapper>& swapper,
                           mc::Buffer** buf )
{
    for(;;)
    {
        *buf = swapper->compositor_acquire();
        if (synchronizer.child_enter_wait()) return;

        swapper->compositor_release(*buf);
        if (synchronizer.child_enter_wait()) return;
        std::this_thread::yield();
    }
}

/* test that the compositor and the client are never in ownership of the same
   buffer */
void BufferSwapperStress::test_distinct_buffers(const std::shared_ptr<mc::BufferSwapper>& swapper)
{
    thread1 = std::thread(compositor_grab_loop, std::ref(compositor_controller), swapper, &compositor_buffer);
    thread2 = std::thread(client_request_loop,  std::ref(client_controller), swapper, &client_buffer);

    for(int i=0; i<  num_iterations; i++)
    {
        compositor_controller.ensure_child_is_waiting();
        client_controller.ensure_child_is_waiting();

        EXPECT_NE(compositor_buffer, client_buffer);

        compositor_controller.activate_waiting_child();
        client_controller.activate_waiting_child();

        main_test_loop_pause(sleep_duration);
    }

    terminate_child_thread(client_controller);
    terminate_child_thread(compositor_controller); 
    thread2.join();
    thread1.join();
}

TEST_F(BufferSwapperStress, distinct_double_buffers_in_client_and_compositor)
{
    test_distinct_buffers(double_swapper);
}
TEST_F(BufferSwapperStress, distinct_triple_buffers_in_client_and_compositor)
{
    test_distinct_buffers(triple_swapper);
}

/* test that we never get an invalid buffer */
void BufferSwapperStress::test_valid_buffers(const std::shared_ptr<mc::BufferSwapper>& swapper)
{
    thread1 = std::thread(compositor_grab_loop, std::ref(compositor_controller), swapper, &compositor_buffer);
    thread2 = std::thread(client_request_loop, std::ref(client_controller), swapper, &client_buffer);

    for(int i=0; i< num_iterations; i++)
    {
        compositor_controller.ensure_child_is_waiting();
        client_controller.ensure_child_is_waiting();

        if (i > 1)
        {
            EXPECT_TRUE((compositor_buffer != NULL));
            EXPECT_TRUE((client_buffer != NULL));
        }

        compositor_controller.activate_waiting_child();
        client_controller.activate_waiting_child();

        main_test_loop_pause(sleep_duration);
    }

    terminate_child_thread(client_controller);
    terminate_child_thread(compositor_controller); 
    thread2.join();
    thread1.join();
}
TEST_F(BufferSwapperStress, double_ensure_valid_buffers)
{
    test_valid_buffers(double_swapper);
}
TEST_F(BufferSwapperStress, triple_ensure_valid_buffers)
{
    test_valid_buffers(triple_swapper);
}

void client_request_loop_finite(std::vector<mc::Buffer*>& buffers,
                      mt::SynchronizerSpawned& synchronizer,
                      const std::shared_ptr<mc::BufferSwapper>& swapper,
                      int const number_of_requests_to_make)
{
    for(auto i=0; i < number_of_requests_to_make; i++)
    {
        auto tmp = swapper->client_acquire();
        swapper->client_release(tmp);
        buffers.push_back(tmp);
    }

    ASSERT_EQ(buffers.size(), (unsigned int) number_of_requests_to_make);
    synchronizer.child_enter_wait();
    synchronizer.child_enter_wait();
}

void compositor_grab(std::vector<mc::Buffer*>& buffers,
                     mt::SynchronizerSpawned& synchronizer,
                     const std::shared_ptr<mc::BufferSwapper>& swapper)
{
    synchronizer.child_enter_wait();

    auto buf = swapper->compositor_acquire();
    swapper->compositor_release(buf);
    buffers.push_back(buf);

    synchronizer.child_enter_wait();
}

void BufferSwapperStress::test_wait_situation(std::vector<mc::Buffer*>& compositor_output_buffers,
                                              std::vector<mc::Buffer*>& client_output_buffers,
                                              const std::shared_ptr<mc::BufferSwapper>& swapper,
                                              unsigned int const number_of_client_requests_to_make)
{
    thread1 = std::thread(compositor_grab, std::ref(compositor_output_buffers),
                          std::ref(compositor_controller), swapper);
    thread2 = std::thread(client_request_loop_finite, std::ref(client_output_buffers),
                          std::ref(client_controller), swapper, number_of_client_requests_to_make);

    compositor_controller.ensure_child_is_waiting();

    client_controller.ensure_child_is_waiting();
    client_controller.activate_waiting_child();

    compositor_controller.activate_waiting_child();
    compositor_controller.ensure_child_is_waiting();

    terminate_child_thread(client_controller);
    terminate_child_thread(compositor_controller); 
    thread2.join();
    thread1.join();

    ASSERT_EQ(client_output_buffers.size(), number_of_client_requests_to_make);
    ASSERT_EQ(compositor_output_buffers.size(), static_cast<unsigned int>(1));
}

TEST_F(BufferSwapperStress, double_test_wait_situation)
{
    std::vector<mc::Buffer*> client_buffers;
    std::vector<mc::Buffer*> compositor_buffers;
    /* a double buffered client should stall on the second request without the compositor running */
    test_wait_situation(compositor_buffers, client_buffers, double_swapper, 2);
    
    EXPECT_EQ(client_buffers.at(0), compositor_buffers.at(0));
}

TEST_F(BufferSwapperStress, triple_test_wait_situation)
{
    std::vector<mc::Buffer*> client_buffers;
    std::vector<mc::Buffer*> compositor_buffers;
    /* a double buffered client should stall on the second request without the compositor running */
    test_wait_situation(compositor_buffers, client_buffers, triple_swapper, 3);
    
    EXPECT_EQ(client_buffers.at(0), compositor_buffers.at(0));
}

void client_request_loop_with_wait(mt::SynchronizerSpawned& synchronizer,
                            const std::shared_ptr<mc::BufferSwapper>& swapper,
                            mc::Buffer** buf )
{
    bool wait_request = false;
    for(;;)
    {
        wait_request = synchronizer.child_check_wait_request();

        *buf = swapper->client_acquire();
        swapper->client_release(*buf);

        if (wait_request)
            if (synchronizer.child_enter_wait()) return;

        std::this_thread::yield();
    }
}

void compositor_grab_loop_with_wait(mt::SynchronizerSpawned& synchronizer,
                            const std::shared_ptr<mc::BufferSwapper>& swapper,
                            mc::Buffer** buf )
{
    bool wait_request = false;
    for(;;)
    {
        wait_request = synchronizer.child_check_wait_request();

        *buf = swapper->compositor_acquire();

        swapper->compositor_release(*buf);

        if (wait_request)
        {
            *buf = swapper->compositor_acquire();
            if (synchronizer.child_enter_wait()) return;
            swapper->compositor_release(*buf);
        }

        std::this_thread::yield();
    }
}

void BufferSwapperStress::test_last_posted(const std::shared_ptr<mc::BufferSwapper>& swapper)
{
    thread1 = std::thread(compositor_grab_loop_with_wait, std::ref(compositor_controller), swapper, &compositor_buffer);
    thread2 = std::thread(client_request_loop_with_wait,  std::ref(client_controller), swapper, &client_buffer);

    for(int i=0; i<  num_iterations; i++)
    {
        client_controller.ensure_child_is_waiting();
        compositor_controller.ensure_child_is_waiting();

        EXPECT_EQ(compositor_buffer, client_buffer);

        compositor_controller.activate_waiting_child();
        client_controller.activate_waiting_child();

        main_test_loop_pause(sleep_duration);
    }

    terminate_child_thread(client_controller);
    terminate_child_thread(compositor_controller); 
    thread2.join();
    thread1.join();
}

TEST_F(BufferSwapperStress, double_test_last_posted)
{
    test_last_posted(double_swapper);
}
}
