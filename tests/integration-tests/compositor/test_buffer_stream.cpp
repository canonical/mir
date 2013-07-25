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
#include "mir/compositor/graphic_buffer_allocator.h"
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
        : buffer_stream{create_bundle()}
    {
    }

    std::shared_ptr<mc::BufferBundle> create_bundle()
    {
        auto allocator = std::make_shared<mtd::StubBufferAllocator>();
        mc::BufferProperties properties{geom::Size{380, 210},
                                        geom::PixelFormat::abgr_8888,
                                        mc::BufferUsage::hardware};

        return std::make_shared<mc::SwitchingBundle>(3, allocator, properties);
    }

    mc::BufferStreamSurfaces buffer_stream;
};

}

TEST_F(BufferStreamTest, gives_only_one_snapshot_buffer)
{
    auto comp1 = buffer_stream.lock_snapshot_buffer();
    auto comp2 = buffer_stream.lock_snapshot_buffer();

    EXPECT_EQ(comp1->id(), comp2->id());

    comp1.reset();

    auto comp3 = buffer_stream.lock_snapshot_buffer();

    EXPECT_EQ(comp2->id(), comp3->id());
}

TEST_F(BufferStreamTest, gives_different_compositor_buffers)
{
    buffer_stream.secure_client_buffer().reset();
    auto comp1 = buffer_stream.lock_compositor_buffer();
    buffer_stream.secure_client_buffer().reset();
    auto comp2 = buffer_stream.lock_compositor_buffer();

    EXPECT_NE(comp1->id(), comp2->id());

    comp1.reset();

    buffer_stream.secure_client_buffer().reset();
    auto comp3 = buffer_stream.lock_compositor_buffer();

    EXPECT_NE(comp2->id(), comp3->id());

    comp2.reset();
    comp3.reset();
}

#if 0 // FIXME - test is no longer valid
TEST_F(BufferStreamTest, multiply_acquired_back_buffer_is_returned_to_client)
{
    buffer_stream.secure_client_buffer().reset();
    auto comp1 = buffer_stream.lock_compositor_buffer();
    buffer_stream.secure_client_buffer().reset();
    auto comp2 = buffer_stream.lock_compositor_buffer();
    buffer_stream.secure_client_buffer().reset();
    auto comp3 = buffer_stream.lock_compositor_buffer();

    EXPECT_NE(comp1->id(), comp2->id());
    EXPECT_NE(comp2->id(), comp3->id());
    EXPECT_NE(comp3->id(), comp1->id());

    auto comp_id = comp1->id();

    comp1.reset();
    comp2.reset();
    comp3.reset();

    auto client1 = buffer_stream.secure_client_buffer();

    EXPECT_EQ(comp_id, client1->id());
}
#endif

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
                     std::atomic<bool> &done,
                     int &composited)
{
    while (!done.load())
    {
        auto comp1 = stream.lock_compositor_buffer();
        ASSERT_NE(nullptr, comp1);
        composited++;
        std::this_thread::yield();

#if 0 // FIXME: Needs a better force_requests_to_complete, or something
        if (done.load())
            break;

        auto comp2 = stream.lock_compositor_buffer();
        ASSERT_NE(nullptr, comp2);
        composited++;
        std::this_thread::yield();

        ASSERT_NE(comp1->id(), comp2->id());
        comp1.reset();
        comp2.reset();
#endif
    }
}

void snapshot_loop(ms::BufferStream &stream,
                   std::atomic<bool> &done,
                   std::atomic<int> &snapshotted)
{
    while (!done.load())
    {
        auto out_region = stream.lock_snapshot_buffer();
        ASSERT_NE(nullptr, out_region);
        snapshotted++;
        std::this_thread::yield();
    }
}

}

TEST_F(BufferStreamTest, stress_test_distinct_buffers)
{
    const int num_snapshotters{3};
    const int num_frames{500};

    std::atomic<bool> done;
    done = false;

    int composited = 0;

    std::atomic<int> snapshotted;
    snapshotted = 0;

    std::thread client(client_loop,
                       num_frames,
                       std::ref(buffer_stream));

    std::thread compositor(compositor_loop,
                           std::ref(buffer_stream),
                           std::ref(done),
                           std::ref(composited));

    std::vector<std::shared_ptr<std::thread>> snapshotters;
    for (unsigned int i = 0; i < num_snapshotters; i++)
    {
        snapshotters.push_back(
            std::make_shared<std::thread>(snapshot_loop,
                                          std::ref(buffer_stream),
                                          std::ref(done),
                                          std::ref(snapshotted)));
    }

    client.join();

    done = true;

    buffer_stream.force_requests_to_complete();

    compositor.join();

    for (auto &s : snapshotters)
        s->join();
}
