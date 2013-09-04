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
#include "mir/graphics/graphic_buffer_allocator.h"
#include "src/server/compositor/switching_bundle.h"

#include "mir_test_doubles/stub_buffer.h"
#include "mir_test_doubles/stub_buffer_allocator.h"
#include "multithread_harness.h"

#include <gmock/gmock.h>

#include <thread>
#include <chrono>
#include <atomic>

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
        : nbuffers{3},
          buffer_stream{create_bundle()}
    {
    }

    std::shared_ptr<mc::BufferBundle> create_bundle()
    {
        auto allocator = std::make_shared<mtd::StubBufferAllocator>();
        mg::BufferProperties properties{geom::Size{380, 210},
                                        geom::PixelFormat::abgr_8888,
                                        mg::BufferUsage::hardware};

        return std::make_shared<mc::SwitchingBundle>(nbuffers,
                                                     allocator,
                                                     properties);
    }

    const int nbuffers;
    mc::BufferStreamSurfaces buffer_stream;
};

}

TEST_F(BufferStreamTest, gives_same_back_buffer_until_more_available)
{
    auto client1 = buffer_stream.secure_client_buffer();
    auto client1_id = client1->id();
    client1.reset();

    auto comp1 = buffer_stream.lock_compositor_buffer(1);
    auto comp2 = buffer_stream.lock_compositor_buffer(1);

    EXPECT_EQ(comp1->id(), comp2->id());
    EXPECT_EQ(comp1->id(), client1_id);

    comp1.reset();

    buffer_stream.secure_client_buffer().reset();
    auto comp3 = buffer_stream.lock_compositor_buffer(2);

    EXPECT_NE(client1_id, comp3->id());

    comp2.reset();
    auto comp3_id = comp3->id();
    comp3.reset();

    auto comp4 = buffer_stream.lock_compositor_buffer(2);
    EXPECT_EQ(comp3_id, comp4->id());
}

TEST_F(BufferStreamTest, gives_all_monitors_the_same_buffer)
{
    for (int i = 0; i < nbuffers - 1; i++)
        buffer_stream.secure_client_buffer().reset();

    auto first_monitor = buffer_stream.lock_compositor_buffer(1);
    auto first_compositor_id = first_monitor->id();
    first_monitor.reset();

    for (int m = 0; m < 10; m++)
    {
        auto monitor = buffer_stream.lock_compositor_buffer(1);
        ASSERT_EQ(first_compositor_id, monitor->id());
    }
}

TEST_F(BufferStreamTest, gives_different_back_buffer_asap)
{
    if (nbuffers > 1)
    {
        buffer_stream.secure_client_buffer().reset();
        auto comp1 = buffer_stream.lock_compositor_buffer(1);

        buffer_stream.secure_client_buffer().reset();
        auto comp2 = buffer_stream.lock_compositor_buffer(2);
    
        EXPECT_NE(comp1->id(), comp2->id());
    
        comp1.reset();
        comp2.reset();
    }
}

TEST_F(BufferStreamTest, can_get_partly_released_back_buffer)
{
    buffer_stream.secure_client_buffer().reset();
    auto client1 = buffer_stream.secure_client_buffer();

    auto comp1 = buffer_stream.lock_compositor_buffer(123);
    auto comp2 = buffer_stream.lock_compositor_buffer(123);

    EXPECT_EQ(comp1->id(), comp2->id());

    comp1.reset();

    auto comp3 = buffer_stream.lock_compositor_buffer(123);

    EXPECT_EQ(comp2->id(), comp3->id());
}

namespace
{

void client_loop(int nframes, ms::BufferStream& stream)
{
    for (int f = 0; f < nframes; f++)
    {
        auto out_buffer = stream.secure_client_buffer();
        ASSERT_NE(nullptr, out_buffer);
        std::this_thread::yield();
    }
}

void compositor_loop(ms::BufferStream &stream,
                     std::atomic<bool> &done)
{
    unsigned long count = 0;

    while (!done.load())
    {
        auto comp1 = stream.lock_compositor_buffer(++count);
        ASSERT_NE(nullptr, comp1);

        // Also stress test getting a second compositor buffer before yielding
        auto comp2 = stream.lock_compositor_buffer(count);
        ASSERT_NE(nullptr, comp2);

        std::this_thread::yield();

        comp1.reset();
        comp2.reset();
    }
}

void snapshot_loop(ms::BufferStream &stream,
                   std::atomic<bool> &done)
{
    while (!done.load())
    {
        auto out_region = stream.lock_snapshot_buffer();
        ASSERT_NE(nullptr, out_region);
        std::this_thread::yield();
    }
}

}

TEST_F(BufferStreamTest, stress_test_distinct_buffers)
{
    // More would be good, but armhf takes too long
    const int num_snapshotters{2};
    const int num_frames{200};

    std::atomic<bool> done;
    done = false;

    std::thread client(client_loop,
                       num_frames,
                       std::ref(buffer_stream));

    std::thread compositor(compositor_loop,
                           std::ref(buffer_stream),
                           std::ref(done));

    std::vector<std::shared_ptr<std::thread>> snapshotters;
    for (unsigned int i = 0; i < num_snapshotters; i++)
    {
        snapshotters.push_back(
            std::make_shared<std::thread>(snapshot_loop,
                                          std::ref(buffer_stream),
                                          std::ref(done)));
    }

    client.join();

    done = true;

    buffer_stream.force_requests_to_complete();

    compositor.join();

    for (auto &s : snapshotters)
        s->join();
}
