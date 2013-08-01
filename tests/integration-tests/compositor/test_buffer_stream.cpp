/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "mir/compositor/buffer_stream_surfaces.h"
#include "mir/compositor/swapper_factory.h"
#include "mir/graphics/graphic_buffer_allocator.h"
#include "src/server/compositor/switching_bundle.h"

#include "mir_test_doubles/stub_buffer.h"
#include "mir_test_doubles/stub_buffer_allocator.h"
#include "multithread_harness.h"

#include <gmock/gmock.h>

#include <thread>
#include <chrono>

namespace mc = mir::compositor;
namespace mg = mir::graphics;
namespace ms = mir::surfaces;
namespace mt = mir::testing;
namespace mtd = mir::test::doubles;
namespace geom = mir::geometry;

namespace
{

struct BufferStreamTest : public ::testing::Test
{
    BufferStreamTest()
        : buffer_stream{create_bundle()}
    {
    }

    std::shared_ptr<mc::BufferBundle> create_bundle()
    {
        auto allocator = std::make_shared<mtd::StubBufferAllocator>();
        auto factory = std::make_shared<mc::SwapperFactory>(allocator);
        mg::BufferProperties properties{geom::Size{380, 210},
                                        geom::PixelFormat::abgr_8888,
                                        mg::BufferUsage::hardware};

        return std::make_shared<mc::SwitchingBundle>(factory, properties);
    }

    void terminate_child_thread(mt::Synchronizer& controller)
    {
        controller.ensure_child_is_waiting();
        controller.kill_thread();
        controller.activate_waiting_child();
    }

    mc::BufferStreamSurfaces buffer_stream;
    std::atomic<bool> client_thread_done;
};

}

TEST_F(BufferStreamTest, gives_same_back_buffer_until_one_is_released)
{
    buffer_stream.secure_client_buffer().reset();
    buffer_stream.secure_client_buffer().reset();

    auto comp1 = buffer_stream.lock_compositor_buffer();
    auto comp2 = buffer_stream.lock_compositor_buffer();

    EXPECT_EQ(comp1->id(), comp2->id());

    comp1.reset();

    auto comp3 = buffer_stream.lock_compositor_buffer();

    EXPECT_NE(comp2->id(), comp3->id());
}

TEST_F(BufferStreamTest, multiply_acquired_back_buffer_is_returned_to_client)
{
    buffer_stream.secure_client_buffer().reset();
    buffer_stream.secure_client_buffer().reset();

    auto comp1 = buffer_stream.lock_compositor_buffer();
    auto comp2 = buffer_stream.lock_compositor_buffer();

    EXPECT_EQ(comp1->id(), comp2->id());

    auto comp_id = comp1->id();

    comp1.reset();
    comp2.reset();

    auto client1 = buffer_stream.secure_client_buffer();

    EXPECT_EQ(comp_id, client1->id());
}

TEST_F(BufferStreamTest, can_get_partly_released_back_buffer)
{
    buffer_stream.secure_client_buffer().reset();
    auto client1 = buffer_stream.secure_client_buffer();

    auto comp1 = buffer_stream.lock_compositor_buffer();
    auto comp2 = buffer_stream.lock_compositor_buffer();

    EXPECT_EQ(comp1->id(), comp2->id());

    comp1.reset();

    auto comp3 = buffer_stream.lock_compositor_buffer();

    EXPECT_EQ(comp2->id(), comp3->id());
}

namespace
{

void client_request_loop(std::shared_ptr<mg::Buffer>& out_buffer,
                         mt::SynchronizerSpawned& synchronizer,
                         ms::BufferStream& stream)
{
    for(;;)
    {
        out_buffer = stream.secure_client_buffer();
        ASSERT_NE(nullptr, out_buffer);

        if (synchronizer.child_enter_wait()) return;

        std::this_thread::yield();
    }
}

void back_buffer_loop(std::shared_ptr<mg::Buffer>& out_region,
                      mt::SynchronizerSpawned& synchronizer,
                      ms::BufferStream& stream)
{
    for(;;)
    {
        out_region = stream.lock_compositor_buffer();
        ASSERT_NE(nullptr, out_region);

        if (synchronizer.child_enter_wait()) return;

        std::this_thread::yield();
    }
}

}

TEST_F(BufferStreamTest, stress_test_distinct_buffers)
{
    unsigned int const num_compositors{3};
    unsigned int const num_iterations{500};
    std::chrono::microseconds const sleep_duration{50};

    struct CompositorInfo
    {
        CompositorInfo(ms::BufferStream& stream)
            : thread{back_buffer_loop, std::ref(back_buffer),
                     std::ref(sync), std::ref(stream)}
        {
        }

        mt::Synchronizer sync;
        std::shared_ptr<mg::Buffer> back_buffer;
        std::thread thread;
    };

    std::vector<std::unique_ptr<CompositorInfo>> compositors;

    mt::Synchronizer client_sync;
    std::shared_ptr<mg::Buffer> client_buffer;
    auto client_thread = std::thread(client_request_loop,  std::ref(client_buffer),
                                     std::ref(client_sync), std::ref(buffer_stream));

    for (unsigned int i = 0; i < num_compositors; i++)
    {
        auto raw = new CompositorInfo{buffer_stream};
        compositors.push_back(std::unique_ptr<CompositorInfo>(raw));
    }

    for(unsigned int i = 0; i < num_iterations; i++)
    {
        /* Pause the threads */
        for (auto& info : compositors)
        {
            info->sync.ensure_child_is_waiting();
        }
        client_sync.ensure_child_is_waiting();

        /*
         * Check that no compositor has the client's buffer, and release
         * all the buffers.
         */
        for (auto& info : compositors)
        {
            EXPECT_NE(info->back_buffer->id(), client_buffer->id());
            info->back_buffer.reset();
        }
        client_buffer.reset();

        /* Restart threads */
        for (auto& info : compositors)
        {
            info->sync.activate_waiting_child();
        }
        client_sync.activate_waiting_child();

        std::this_thread::sleep_for(sleep_duration);
    }

    terminate_child_thread(client_sync);
    client_thread.join();

    for (auto& info : compositors)
    {
        terminate_child_thread(info->sync);
        info->thread.join();
    }
}
