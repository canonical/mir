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

#include "mir_test_doubles/stub_buffer.h"
#include "multithread_harness.h"

#include "mir/chrono/chrono.h"
#include "mir/compositor/buffer_swapper_multi.h"
#include "mir/compositor/buffer_id.h"
#include "mir/thread/all.h"

namespace mc = mir::compositor;
namespace mt = mir::testing;
namespace geom = mir::geometry;
namespace mtd = mir::test::doubles;

namespace mir
{
struct BufferSwapperStress : public ::testing::Test
{
public:
    BufferSwapperStress()
     : sleep_duration(50),
       num_iterations(500)
    {
        buffer_a = std::make_shared<mtd::StubBuffer>();
        buffer_b = std::make_shared<mtd::StubBuffer>();
        buffer_c = std::make_shared<mtd::StubBuffer>();
    }

    void terminate_child_thread(mt::Synchronizer& controller)
    {
        controller.ensure_child_is_waiting();
        controller.kill_thread();
        controller.activate_waiting_child();
    }

    mt::Synchronizer compositor_controller;
    mt::Synchronizer client_controller;

    std::chrono::microseconds const sleep_duration;
    int const num_iterations;

    std::thread thread1;
    std::thread thread2;

    void test_distinct_buffers(mc::BufferSwapper& swapper);
    void test_valid_buffers(mc::BufferSwapper& swapper);
    void test_wait_situation(std::vector<mc::BufferID>& compositor_output_buffers,
                             std::vector<mc::BufferID>& client_output_buffers,
                             mc::BufferSwapper& swapper,
                             unsigned int number_of_client_requests_to_make);
    void test_last_posted(mc::BufferSwapper& swapper);

    std::shared_ptr<mc::Buffer> buffer_a;
    std::shared_ptr<mc::Buffer> buffer_b;
    std::shared_ptr<mc::Buffer> buffer_c;
};

void main_test_loop_pause(std::chrono::microseconds duration) {
    std::this_thread::sleep_for(duration);
}

void client_request_loop(mc::BufferID& out_buffer_id, mt::SynchronizerSpawned& synchronizer,
                         mc::BufferSwapper& swapper)
{
    std::shared_ptr<mc::Buffer> buffer_ref;
    for(;;)
    {
        swapper.client_acquire(buffer_ref, out_buffer_id);
        EXPECT_NE(nullptr, buffer_ref);
        if (synchronizer.child_enter_wait()) return;

        swapper.client_release(out_buffer_id);
        if (synchronizer.child_enter_wait()) return;

        std::this_thread::yield();
    }
}

void compositor_grab_loop(mc::BufferID& out_buffer_id, mt::SynchronizerSpawned& synchronizer,
                          mc::BufferSwapper& swapper)
{
    std::shared_ptr<mc::Buffer> buffer_ref;
    for(;;)
    {
        swapper.compositor_acquire(buffer_ref, out_buffer_id);
        EXPECT_NE(nullptr, buffer_ref);
        if (synchronizer.child_enter_wait()) return;

        swapper.compositor_release(out_buffer_id);
        if (synchronizer.child_enter_wait()) return;
        std::this_thread::yield();
    }
}

/* test that the compositor and the client are never in ownership of the same
   buffer */
void BufferSwapperStress::test_distinct_buffers(mc::BufferSwapper& swapper)
{
    mc::BufferID compositor_buffer_id;
    mc::BufferID client_buffer_id;

    thread1 = std::thread(compositor_grab_loop, std::ref(compositor_buffer_id),
                          std::ref(compositor_controller), std::ref(swapper));
    thread2 = std::thread(client_request_loop,  std::ref(client_buffer_id),
                          std::ref(client_controller), std::ref(swapper));

    for(int i=0; i<  num_iterations; i++)
    {
        compositor_controller.ensure_child_is_waiting();
        client_controller.ensure_child_is_waiting();

        EXPECT_NE(compositor_buffer_id, client_buffer_id);

        compositor_controller.activate_waiting_child();
        client_controller.activate_waiting_child();

        main_test_loop_pause(sleep_duration);
    }

    terminate_child_thread(client_controller);
    terminate_child_thread(compositor_controller);
    thread2.join();
    thread1.join();
}

TEST_F(BufferSwapperStress, distinct_and_valid_double_buffers_in_client_and_compositor)
{
    auto double_list = std::initializer_list<std::shared_ptr<mc::Buffer>>{buffer_a, buffer_b};
    mc::BufferSwapperMulti double_swapper(double_list);
    test_distinct_buffers(double_swapper);
}

TEST_F(BufferSwapperStress, distinct_and_valid_triple_buffers_in_client_and_compositor)
{
    auto triple_list = std::initializer_list<std::shared_ptr<mc::Buffer>>{buffer_a, buffer_b, buffer_c};
    mc::BufferSwapperMulti triple_swapper(triple_list);
    test_distinct_buffers(triple_swapper);
}

void client_request_loop_finite(std::vector<mc::BufferID>& buffers,
                      mt::SynchronizerSpawned& synchronizer,
                      mc::BufferSwapper& swapper,
                      int const number_of_requests_to_make)
{
    std::shared_ptr<mc::Buffer> buffer_ref;
    mc::BufferID tmp;
    for(auto i=0; i < number_of_requests_to_make; i++)
    {
        swapper.client_acquire(buffer_ref, tmp);
        swapper.client_release(tmp);
        buffers.push_back(tmp);
    }

    synchronizer.child_enter_wait();
    synchronizer.child_enter_wait();
}

void compositor_grab(std::vector<mc::BufferID>& buffers,
                     mt::SynchronizerSpawned& synchronizer,
                     mc::BufferSwapper& swapper)
{
    std::shared_ptr<mc::Buffer> buffer_ref;
    mc::BufferID tmp;

    synchronizer.child_enter_wait();

    swapper.compositor_acquire(buffer_ref, tmp);
    swapper.compositor_release(tmp);
    buffers.push_back(tmp);

    synchronizer.child_enter_wait();
}

void BufferSwapperStress::test_wait_situation(std::vector<mc::BufferID>& compositor_output_buffers,
                                              std::vector<mc::BufferID>& client_output_buffers,
                                              mc::BufferSwapper& swapper,
                                              unsigned int const number_of_client_requests_to_make)
{
    thread1 = std::thread(compositor_grab, std::ref(compositor_output_buffers),
                          std::ref(compositor_controller), std::ref(swapper));
    thread2 = std::thread(client_request_loop_finite, std::ref(client_output_buffers),
                          std::ref(client_controller), std::ref(swapper), number_of_client_requests_to_make);

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
    std::vector<mc::BufferID> client_buffers;
    std::vector<mc::BufferID> compositor_buffers;
    /* a double buffered client should stall on the second request without the compositor running */
    auto double_list = std::initializer_list<std::shared_ptr<mc::Buffer>>{buffer_a, buffer_b};
    mc::BufferSwapperMulti double_swapper(double_list);
    test_wait_situation(compositor_buffers, client_buffers, double_swapper, 2);

    EXPECT_EQ(client_buffers.at(0), compositor_buffers.at(0));
}

TEST_F(BufferSwapperStress, triple_test_wait_situation)
{
    std::vector<mc::BufferID> client_buffers;
    std::vector<mc::BufferID> compositor_buffers;
    auto triple_list = std::initializer_list<std::shared_ptr<mc::Buffer>>{buffer_a, buffer_b, buffer_c};
    mc::BufferSwapperMulti triple_swapper(triple_list);
    /* a triple buffered client should stall on the third request without the compositor running */
    test_wait_situation(compositor_buffers, client_buffers, triple_swapper, 3);

    EXPECT_EQ(client_buffers.at(0), compositor_buffers.at(0));
}

void client_request_loop_with_wait(mc::BufferID& out_buffer_id, mt::SynchronizerSpawned& synchronizer,
                                   mc::BufferSwapper& swapper)
{
    std::shared_ptr<mc::Buffer> buffer_ref;
    bool wait_request = false;
    for(;;)
    {
        wait_request = synchronizer.child_check_wait_request();

        swapper.client_acquire(buffer_ref, out_buffer_id);
        swapper.client_release(out_buffer_id);

        if (wait_request)
            if (synchronizer.child_enter_wait()) return;

        std::this_thread::yield();
    }
}

void compositor_grab_loop_with_wait(mc::BufferID& out_buffer_id, mt::SynchronizerSpawned& synchronizer,
                                    mc::BufferSwapper& swapper)
{
    std::shared_ptr<mc::Buffer> buffer_ref;
    bool wait_request = false;
    for(;;)
    {
        wait_request = synchronizer.child_check_wait_request();

        swapper.compositor_acquire(buffer_ref, out_buffer_id);
        swapper.compositor_release(out_buffer_id);

        if (wait_request)
        {
            swapper.compositor_acquire(buffer_ref, out_buffer_id);
            if (synchronizer.child_enter_wait()) return;
            swapper.compositor_release(out_buffer_id);
        }

        std::this_thread::yield();
    }
}

void BufferSwapperStress::test_last_posted(mc::BufferSwapper& swapper)
{
    mc::BufferID compositor_buffer;
    mc::BufferID client_buffer;

    thread1 = std::thread(compositor_grab_loop_with_wait, std::ref(compositor_buffer), std::ref(compositor_controller), std::ref(swapper));
    thread2 = std::thread(client_request_loop_with_wait,  std::ref(client_buffer), std::ref(client_controller), std::ref(swapper));

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
    auto double_list = std::initializer_list<std::shared_ptr<mc::Buffer>>{buffer_a, buffer_b};
    mc::BufferSwapperMulti double_swapper(double_list);
    test_last_posted(double_swapper);
}
}
