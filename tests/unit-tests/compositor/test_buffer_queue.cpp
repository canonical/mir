/*
 * Copyright © 2013-2014 Canonical Ltd.
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
 * Authored by: Daniel van Vugt <daniel.van.vugt@canonical.com>
 *              Alberto Aguirre <alberto.aguirre@canonical.com>
 */

#include "src/server/compositor/buffer_queue.h"
#include "mir_test_doubles/stub_buffer_allocator.h"
#include "mir_test_doubles/stub_buffer.h"
#include "mir_test/auto_unblock_thread.h"
#include "mir_test/wait_condition.h"

#include <gtest/gtest.h>

#include <memory>
#include <atomic>
#include <mutex>
#include <chrono>
#include <deque>
#include <unordered_set>

namespace geom = mir::geometry;
namespace mtd = mir::test::doubles;
namespace mt = mir::test;
namespace mc=mir::compositor;
namespace mg = mir::graphics;

using namespace testing;

namespace
{
class BufferQueueTest : public ::testing::Test
{
public:
    BufferQueueTest()
        : max_nbuffers_to_test{5}
    {};

    void SetUp()
    {
        allocator = std::make_shared<mtd::StubBufferAllocator>();
        basic_properties =
        {
            geom::Size{3, 4},
            mir_pixel_format_abgr_8888,
            mg::BufferUsage::hardware
        };
    }
protected:
    int max_nbuffers_to_test;
    std::shared_ptr<mtd::StubBufferAllocator> allocator;
    mg::BufferProperties basic_properties;
};

class AcquireWaitHandle
{
public:
    AcquireWaitHandle(mc::BufferQueue& q)
        : buffer_{nullptr}, q{&q}, received_buffer{false}
    {}

    void receive_buffer(mg::Buffer* new_buffer)
    {
        std::lock_guard<decltype(guard)> lock(guard);
        buffer_ = new_buffer;
        received_buffer = true;
        cv.notify_one();
    }

    void wait()
    {
        std::unique_lock<decltype(guard)> lock(guard);
        cv.wait(lock, [&]{ return received_buffer; });
    }

    template<typename Rep, typename Period>
    bool wait_for(std::chrono::duration<Rep, Period> const& duration)
    {
        std::unique_lock<decltype(guard)> lock(guard);
        return cv.wait_for(lock, duration, [&]{ return received_buffer; });
    }

    bool has_acquired_buffer()
    {
        std::lock_guard<decltype(guard)> lock(guard);
        return received_buffer;
    }

    void release_buffer()
    {
        if (buffer_)
        {
            q->client_release(buffer_);
            received_buffer = false;
        }
    }

    mg::BufferID id()
    {
        return buffer_->id();
    }

    mg::Buffer* buffer()
    {
        return buffer_;
    }

private:
    mg::Buffer* buffer_;
    mc::BufferQueue* q;
    std::condition_variable cv;
    std::mutex guard;
    bool received_buffer;
};

std::shared_ptr<AcquireWaitHandle> client_acquire_async(mc::BufferQueue& q)
{
    std::shared_ptr<AcquireWaitHandle> wait_handle =
        std::make_shared<AcquireWaitHandle>(q);

    q.client_acquire(
        [wait_handle](mg::Buffer* buffer) { wait_handle->receive_buffer(buffer); });

    return wait_handle;
}

mg::Buffer* client_acquire_sync(mc::BufferQueue& q)
{
    auto handle = client_acquire_async(q);
    handle->wait();
    return handle->buffer();
}

void compositor_thread(mc::BufferQueue &bundle, std::atomic<bool> &done)
{
   while (!done)
   {
       bundle.compositor_release(bundle.compositor_acquire(nullptr));
       std::this_thread::yield();
   }
}

void snapshot_thread(mc::BufferQueue &bundle,
                      std::atomic<bool> &done)
{
   while (!done)
   {
       bundle.snapshot_release(bundle.snapshot_acquire());
       std::this_thread::yield();
   }
}

void client_thread(mc::BufferQueue &bundle, int nframes)
{
   for (int i = 0; i < nframes; i++)
   {
       bundle.client_release(client_acquire_sync(bundle));
       std::this_thread::yield();
   }
}

void switching_client_thread(mc::BufferQueue &bundle, int nframes)
{
   bool enable_frame_dropping{false};
   int const nframes_to_test_before_switching{5};
   for (int i = 0; i < nframes; ++i)
   {
       bundle.allow_framedropping(enable_frame_dropping);
       for (int j = 0; j < nframes_to_test_before_switching; j++)
           bundle.client_release(client_acquire_sync(bundle));
       enable_frame_dropping = !enable_frame_dropping;
       std::this_thread::yield();
   }
}
}

TEST_F(BufferQueueTest, buffer_queue_of_one_is_supported)
{
    std::unique_ptr<mc::BufferQueue> q;
    ASSERT_NO_THROW(q = std::move(
        std::unique_ptr<mc::BufferQueue>(
            new mc::BufferQueue(1, allocator, basic_properties))));
    ASSERT_THAT(q, Ne(nullptr));

    auto handle = client_acquire_async(*q);

    /* Client is allowed to get the only buffer in existence */
    ASSERT_THAT(handle->has_acquired_buffer(), Eq(true));

    /* Client blocks until the client releases
     * the buffer and compositor composites it*/
    auto next_request = client_acquire_async(*q);
    EXPECT_THAT(next_request->has_acquired_buffer(), Eq(false));

    auto comp_buffer = q->compositor_acquire(this);
    auto client_id = handle->id();

    /* Client and compositor always share the same buffer */
    EXPECT_THAT(client_id, Eq(comp_buffer->id()));

    EXPECT_NO_THROW(handle->release_buffer());
    EXPECT_NO_THROW(q->compositor_release(comp_buffer));

    /* Simulate a composite pass */
    comp_buffer = q->compositor_acquire(this);
    q->compositor_release(comp_buffer);

    /* The request should now be fullfilled after compositor
     * released the buffer
     */
    EXPECT_THAT(next_request->has_acquired_buffer(), Eq(true));
    EXPECT_NO_THROW(next_request->release_buffer());
}

TEST_F(BufferQueueTest, buffer_queue_of_one_supports_resizing)
{
    mc::BufferQueue q(1, allocator, basic_properties);

    const geom::Size expect_size{10, 20};
    q.resize(expect_size);

    auto handle = client_acquire_async(q);
    ASSERT_THAT(handle->has_acquired_buffer(), Eq(true));
    auto buffer = handle->buffer();
    ASSERT_THAT(buffer->size(), Eq(expect_size));

    /* Client and compositor share the same buffer so
     * expect the new size
     */
    std::shared_ptr<mg::Buffer> comp_buffer;
    ASSERT_NO_THROW(comp_buffer = q.compositor_acquire(this));

    EXPECT_THAT(buffer->size(), Eq(expect_size));
    EXPECT_NO_THROW(q.compositor_release(comp_buffer));

    EXPECT_NO_THROW(handle->release_buffer());
    EXPECT_NO_THROW(q.compositor_release(q.compositor_acquire(this)));
}

TEST_F(BufferQueueTest, framedropping_is_disabled_by_default)
{
    mc::BufferQueue bundle(2, allocator, basic_properties);
    EXPECT_THAT(bundle.framedropping_allowed(), Eq(false));
}

TEST_F(BufferQueueTest, throws_when_creating_with_invalid_num_buffers)
{
    EXPECT_THROW(mc::BufferQueue a(0, allocator, basic_properties), std::logic_error);
    EXPECT_THROW(mc::BufferQueue a(-1, allocator, basic_properties), std::logic_error);
    EXPECT_THROW(mc::BufferQueue a(-10, allocator, basic_properties), std::logic_error);
}

TEST_F(BufferQueueTest, client_can_acquire_and_release_buffer)
{
    for (int nbuffers = 1; nbuffers <= max_nbuffers_to_test; ++nbuffers)
    {
        mc::BufferQueue q(nbuffers, allocator, basic_properties);

        auto handle = client_acquire_async(q);
        ASSERT_THAT(handle->has_acquired_buffer(), Eq(true));
        ASSERT_NO_THROW(handle->release_buffer());
    }
}

TEST_F(BufferQueueTest, client_can_acquire_buffers)
{
    for (int nbuffers = 2; nbuffers <= max_nbuffers_to_test; ++nbuffers)
    {
        mc::BufferQueue q(nbuffers, allocator, basic_properties);
        int const max_ownable_buffers = nbuffers - 1;
        for (int acquires = 0; acquires < max_ownable_buffers; ++acquires)
        {
            auto handle = client_acquire_async(q);
            ASSERT_THAT(handle->has_acquired_buffer(), Eq(true));
        }
    }
}

/* Regression test for LP: #1315302 */
TEST_F(BufferQueueTest, clients_can_have_multiple_pending_completions)
{
    int const nbuffers = 3;
    mc::BufferQueue q(nbuffers, allocator, basic_properties);

    for (int i = 0; i < nbuffers - 1; ++i)
    {
        auto handle = client_acquire_async(q);
        ASSERT_THAT(handle->has_acquired_buffer(), Eq(true));
        handle->release_buffer();
    }

    auto handle1 = client_acquire_async(q);
    ASSERT_THAT(handle1->has_acquired_buffer(), Eq(false));

    auto handle2 = client_acquire_async(q);
    ASSERT_THAT(handle1->has_acquired_buffer(), Eq(false));

    for (int i = 0; i < nbuffers + 1; ++i)
        q.compositor_release(q.compositor_acquire(this));

    EXPECT_THAT(handle1->has_acquired_buffer(), Eq(true));
    EXPECT_THAT(handle2->has_acquired_buffer(), Eq(true));
    EXPECT_THAT(handle1->buffer(), Ne(handle2->buffer()));

    handle1->release_buffer();
    handle2->release_buffer();
}

TEST_F(BufferQueueTest, compositor_acquires_frames_in_order_for_synchronous_client)
{
    for (int nbuffers = 2; nbuffers <= max_nbuffers_to_test; ++nbuffers)
    {
        mc::BufferQueue q(nbuffers, allocator, basic_properties);
        ASSERT_THAT(q.framedropping_allowed(), Eq(false));

        void const* main_compositor = reinterpret_cast<void const*>(0);
        void const* second_compositor = reinterpret_cast<void const*>(1);
        for (int i = 0; i < 20; i++)
        {
            auto handle = client_acquire_async(q);
            ASSERT_THAT(handle->has_acquired_buffer(), Eq(true));
            auto client_id = handle->id();
            handle->release_buffer();

            auto comp_buffer = q.compositor_acquire(main_compositor);
            auto composited_id = comp_buffer->id();
            q.compositor_release(comp_buffer);

            EXPECT_THAT(composited_id, Eq(client_id));

            comp_buffer = q.compositor_acquire(second_compositor);
            EXPECT_THAT(composited_id, Eq(comp_buffer->id()));
            q.compositor_release(comp_buffer);
        }
    }
}

TEST_F(BufferQueueTest, framedropping_clients_never_block)
{
    for (int nbuffers = 2; nbuffers <= max_nbuffers_to_test; ++nbuffers)
    {
        mc::BufferQueue q(nbuffers, allocator, basic_properties);
        q.allow_framedropping(true);

        for (int i = 0; i < 1000; i++)
        {
            auto handle = client_acquire_async(q);
            ASSERT_THAT(handle->has_acquired_buffer(), Eq(true));
            handle->release_buffer();
        }
    }
}

/* Regression test for LP: #1210042 */
TEST_F(BufferQueueTest, clients_dont_recycle_startup_buffer)
{
    mc::BufferQueue q(3, allocator, basic_properties);

    auto handle = client_acquire_async(q);
    ASSERT_THAT(handle->has_acquired_buffer(), Eq(true));
    auto client_id = handle->id();
    handle->release_buffer();

    handle = client_acquire_async(q);
    ASSERT_THAT(handle->has_acquired_buffer(), Eq(true));
    handle->release_buffer();

    auto comp_buffer = q.compositor_acquire(this);
    EXPECT_THAT(client_id, Eq(comp_buffer->id()));
    q.compositor_release(comp_buffer);
}

TEST_F(BufferQueueTest, throws_on_out_of_order_client_release)
{
    for (int nbuffers = 3; nbuffers <= max_nbuffers_to_test; ++nbuffers)
    {
        mc::BufferQueue q(nbuffers, allocator, basic_properties);

        auto handle1 = client_acquire_async(q);
        ASSERT_THAT(handle1->has_acquired_buffer(), Eq(true));

        auto handle2 = client_acquire_async(q);
        ASSERT_THAT(handle2->has_acquired_buffer(), Eq(true));

        EXPECT_THROW(handle2->release_buffer(), std::logic_error);
        EXPECT_NO_THROW(handle1->release_buffer());

        EXPECT_THROW(handle1->release_buffer(), std::logic_error);
        EXPECT_NO_THROW(handle2->release_buffer());
    }
}

TEST_F(BufferQueueTest, async_client_cycles_through_all_buffers)
{
    for (int nbuffers = 2; nbuffers <= max_nbuffers_to_test; ++nbuffers)
    {
        mc::BufferQueue q(nbuffers, allocator, basic_properties);

        std::atomic<bool> done(false);
        auto unblock = [&done] { done = true; };
        mt::AutoUnblockThread compositor(unblock,
            compositor_thread, std::ref(q), std::ref(done));

        std::unordered_set<uint32_t> ids_acquired;
        int const max_ownable_buffers = nbuffers - 1;
        for (int i = 0; i < max_ownable_buffers*2; ++i)
        {
            std::vector<mg::Buffer *> client_buffers;
            for (int acquires = 0; acquires < max_ownable_buffers; ++acquires)
            {
                auto handle = client_acquire_async(q);
                handle->wait_for(std::chrono::seconds(1));
                ASSERT_THAT(handle->has_acquired_buffer(), Eq(true));
                ids_acquired.insert(handle->id().as_uint32_t());
                client_buffers.push_back(handle->buffer());
            }

            for (auto const& buffer : client_buffers)
            {
                q.client_release(buffer);
            }
        }

        EXPECT_THAT(ids_acquired.size(), Eq(nbuffers));
    }
}

TEST_F(BufferQueueTest, compositor_can_acquire_and_release)
{
    for (int nbuffers = 1; nbuffers <= max_nbuffers_to_test; ++nbuffers)
    {
        mc::BufferQueue q(nbuffers, allocator, basic_properties);

        auto handle = client_acquire_async(q);
        ASSERT_THAT(handle->has_acquired_buffer(), Eq(true));

        auto client_id = handle->id();
        ASSERT_NO_THROW(handle->release_buffer());

        auto comp_buffer = q.compositor_acquire(this);
        EXPECT_THAT(client_id, Eq(comp_buffer->id()));
        EXPECT_NO_THROW(q.compositor_release(comp_buffer));
    }
}

TEST_F(BufferQueueTest, multiple_compositors_are_in_sync)
{
    for (int nbuffers = 1; nbuffers <= max_nbuffers_to_test; ++nbuffers)
    {
        mc::BufferQueue q(nbuffers, allocator, basic_properties);

        auto handle = client_acquire_async(q);
        ASSERT_THAT(handle->has_acquired_buffer(), Eq(true));

        auto client_id = handle->id();
        ASSERT_NO_THROW(handle->release_buffer());

        for (int monitor = 0; monitor < 10; monitor++)
        {
            void const* user_id = reinterpret_cast<void const*>(monitor);
            auto comp_buffer = q.compositor_acquire(user_id);
            EXPECT_THAT(client_id, Eq(comp_buffer->id()));
            q.compositor_release(comp_buffer);
        }
    }
}

TEST_F(BufferQueueTest, compositor_acquires_frames_in_order)
{
    for (int nbuffers = 2; nbuffers <= max_nbuffers_to_test; ++nbuffers)
    {
        mc::BufferQueue q(nbuffers, allocator, basic_properties);

        for (int i = 0; i < 10; ++i)
        {
            std::deque<mg::BufferID> client_release_sequence;
            std::vector<mg::Buffer *> buffers;
            int const max_ownable_buffers = nbuffers - 1;
            for (int i = 0; i < max_ownable_buffers; ++i)
            {
                auto handle = client_acquire_async(q);
                ASSERT_THAT(handle->has_acquired_buffer(), Eq(true));
                buffers.push_back(handle->buffer());
            }

            for (auto buffer : buffers)
            {
                client_release_sequence.push_back(buffer->id());
                q.client_release(buffer);
            }

            for (auto const& client_id : client_release_sequence)
            {
                auto comp_buffer = q.compositor_acquire(this);
                EXPECT_THAT(client_id, Eq(comp_buffer->id()));
                q.compositor_release(comp_buffer);
            }
        }
    }
}

TEST_F(BufferQueueTest, compositor_acquire_never_blocks_when_there_are_no_ready_buffers)
{
    for (int nbuffers = 1; nbuffers <= max_nbuffers_to_test; ++nbuffers)
    {
        mc::BufferQueue q(nbuffers, allocator, basic_properties);

        for (int i = 0; i < 100; i++)
        {
            auto buffer = q.compositor_acquire(this);
            q.compositor_release(buffer);
        }
    }
}

TEST_F(BufferQueueTest, compositor_can_always_acquire_buffer)
{
    for (int nbuffers = 2; nbuffers <= max_nbuffers_to_test; ++nbuffers)
    {
        mc::BufferQueue q(nbuffers, allocator, basic_properties);
        q.allow_framedropping(false);

        std::atomic<bool> done(false);
        auto unblock = [&done] { done = true; };

        mt::AutoJoinThread client([&q]
        {
            for (int nframes = 0; nframes < 100; ++nframes)
            {
                auto handle = client_acquire_async(q);
                handle->wait_for(std::chrono::seconds(1));
                ASSERT_THAT(handle->has_acquired_buffer(), Eq(true));
                handle->release_buffer();
                std::this_thread::yield();
            }
        });
        mt::AutoUnblockThread compositor(unblock, [&]
        {
            while (!done)
            {
                std::shared_ptr<mg::Buffer> buffer;
                EXPECT_NO_THROW(buffer = q.compositor_acquire(this));
                EXPECT_THAT(buffer, Ne(nullptr));
                EXPECT_NO_THROW(q.compositor_release(buffer));
                std::this_thread::yield();
            }
        });

        client.stop();
        compositor.stop();
    }
}

TEST_F(BufferQueueTest, compositor_acquire_recycles_latest_ready_buffer)
{
    for (int nbuffers = 1; nbuffers <= max_nbuffers_to_test; ++nbuffers)
    {
        mc::BufferQueue q(nbuffers, allocator, basic_properties);

        mg::BufferID client_id;

        for (int i = 0; i < 20; i++)
        {
            if (i % 10 == 0)
            {
                auto handle = client_acquire_async(q);
                ASSERT_THAT(handle->has_acquired_buffer(), Eq(true));
                client_id = handle->id();
                handle->release_buffer();
            }

            for (int monitor_id = 0; monitor_id < 10; monitor_id++)
            {
                void const* user_id = reinterpret_cast<void const*>(monitor_id);
                auto buffer = q.compositor_acquire(user_id);
                ASSERT_THAT(buffer->id(), Eq(client_id));
                q.compositor_release(buffer);
            }
        }
    }
}

TEST_F(BufferQueueTest, compositor_release_verifies_parameter)
{
    for (int nbuffers = 1; nbuffers <= max_nbuffers_to_test; ++nbuffers)
    {
        mc::BufferQueue q(nbuffers, allocator, basic_properties);

        auto handle = client_acquire_async(q);
        ASSERT_THAT(handle->has_acquired_buffer(), Eq(true));
        handle->release_buffer();

        auto comp_buffer = q.compositor_acquire(this);
        q.compositor_release(comp_buffer);
        EXPECT_THROW(q.compositor_release(comp_buffer), std::logic_error);
    }
}

/* Regression test for LP#1270964 */
TEST_F(BufferQueueTest, compositor_client_interleaved)
{
    mc::BufferQueue q(3, allocator, basic_properties);

    auto handle = client_acquire_async(q);
    ASSERT_THAT(handle->has_acquired_buffer(), Eq(true));

    auto first_ready_buffer_id = handle->id();
    handle->release_buffer();

    handle = client_acquire_async(q);
    ASSERT_THAT(handle->has_acquired_buffer(), Eq(true));

    // in the original bug, compositor would be given the wrong buffer here
    auto compositor_buffer = q.compositor_acquire(this);

    EXPECT_THAT(compositor_buffer->id(), Eq(first_ready_buffer_id));

    handle->release_buffer();
    q.compositor_release(compositor_buffer);
}

TEST_F(BufferQueueTest, overlapping_compositors_get_different_frames)
{
    // This test simulates bypass behaviour
    for (int nbuffers = 2; nbuffers <= max_nbuffers_to_test; ++nbuffers)
    {
        mc::BufferQueue q(nbuffers, allocator, basic_properties);

        std::shared_ptr<mg::Buffer> compositor[2];

        auto handle = client_acquire_async(q);
        ASSERT_THAT(handle->has_acquired_buffer(), Eq(true));
        handle->release_buffer();
        compositor[0] = q.compositor_acquire(this);

        handle = client_acquire_async(q);
        ASSERT_THAT(handle->has_acquired_buffer(), Eq(true));
        handle->release_buffer();
        compositor[1] = q.compositor_acquire(this);

        for (int i = 0; i < 20; i++)
        {
            // Two compositors acquired, and they're always different...
            ASSERT_THAT(compositor[0]->id(), Ne(compositor[1]->id()));

            // One of the compositors (the oldest one) gets a new buffer...
            int oldest = i & 1;
            q.compositor_release(compositor[oldest]);
            auto handle = client_acquire_async(q);
            ASSERT_THAT(handle->has_acquired_buffer(), Eq(true));
            handle->release_buffer();
            compositor[oldest] = q.compositor_acquire(this);
        }

        q.compositor_release(compositor[0]);
        q.compositor_release(compositor[1]);
    }
}

TEST_F(BufferQueueTest, snapshot_acquire_basic)
{
    for (int nbuffers = 1; nbuffers <= max_nbuffers_to_test; ++nbuffers)
    {
        mc::BufferQueue q(nbuffers, allocator, basic_properties);

        auto comp_buffer = q.compositor_acquire(this);
        auto snapshot = q.snapshot_acquire();
        EXPECT_THAT(snapshot->id(), Eq(comp_buffer->id()));
        q.compositor_release(comp_buffer);
        q.snapshot_release(snapshot);
    }
}

TEST_F(BufferQueueTest, snapshot_acquire_never_blocks)
{
    for (int nbuffers = 1; nbuffers <= max_nbuffers_to_test; ++nbuffers)
    {
        mc::BufferQueue q(nbuffers, allocator, basic_properties);
        int const num_snapshots = 100;

        std::shared_ptr<mg::Buffer> buf[num_snapshots];
        for (int i = 0; i < num_snapshots; i++)
            buf[i] = q.snapshot_acquire();

        for (int i = 0; i < num_snapshots; i++)
            q.snapshot_release(buf[i]);
    }
}

TEST_F(BufferQueueTest, snapshot_release_verifies_parameter)
{
    for (int nbuffers = 2; nbuffers <= max_nbuffers_to_test; ++nbuffers)
    {
        mc::BufferQueue q(nbuffers, allocator, basic_properties);

        auto handle = client_acquire_async(q);
        ASSERT_THAT(handle->has_acquired_buffer(), Eq(true));
        handle->release_buffer();

        auto comp_buffer = q.compositor_acquire(this);
        EXPECT_THROW(q.snapshot_release(comp_buffer), std::logic_error);

        handle = client_acquire_async(q);
        ASSERT_THAT(handle->has_acquired_buffer(), Eq(true));
        auto snapshot = q.snapshot_acquire();

        EXPECT_THAT(snapshot->id(), Eq(comp_buffer->id()));
        EXPECT_THAT(snapshot->id(), Ne(handle->id()));
        EXPECT_NO_THROW(q.snapshot_release(snapshot));
        EXPECT_THROW(q.snapshot_release(snapshot), std::logic_error);
    }
}

TEST_F(BufferQueueTest, stress)
{
    for (int nbuffers = 2; nbuffers <= max_nbuffers_to_test; ++nbuffers)
    {
        mc::BufferQueue q(nbuffers, allocator, basic_properties);

        std::atomic<bool> done(false);

        auto unblock = [&done]{ done = true;};

        mt::AutoUnblockThread compositor(unblock, compositor_thread,
                                         std::ref(q),
                                         std::ref(done));
        mt::AutoUnblockThread snapshotter1(unblock, snapshot_thread,
                                           std::ref(q),
                                           std::ref(done));
        mt::AutoUnblockThread snapshotter2(unblock, snapshot_thread,
                                           std::ref(q),
                                           std::ref(done));

        q.allow_framedropping(false);
        mt::AutoJoinThread client1(client_thread, std::ref(q), 1000);
        client1.stop();

        q.allow_framedropping(true);
        mt::AutoJoinThread client2(client_thread, std::ref(q), 1000);
        client2.stop();

        mt::AutoJoinThread client3(switching_client_thread, std::ref(q), 1000);
        client3.stop();
    }
}

TEST_F(BufferQueueTest, bypass_clients_get_more_than_two_buffers)
{
    for (int nbuffers = 3; nbuffers <= max_nbuffers_to_test; ++nbuffers)
    {
        mc::BufferQueue q(nbuffers, allocator, basic_properties);

        std::shared_ptr<mg::Buffer> compositor[2];

        auto handle = client_acquire_async(q);
        ASSERT_THAT(handle->has_acquired_buffer(), Eq(true));
        handle->release_buffer();
        compositor[0] = q.compositor_acquire(this);

        handle = client_acquire_async(q);
        ASSERT_THAT(handle->has_acquired_buffer(), Eq(true));
        handle->release_buffer();
        compositor[1] = q.compositor_acquire(this);

        for (int i = 0; i < 20; i++)
        {
            // Two compositors acquired, and they're always different...
            ASSERT_THAT(compositor[0]->id(), Ne(compositor[1]->id()));

            handle = client_acquire_async(q);
            ASSERT_THAT(handle->has_acquired_buffer(), Eq(true));
            ASSERT_THAT(compositor[0]->id(), Ne(handle->id()));
            ASSERT_THAT(compositor[1]->id(), Ne(handle->id()));
            handle->release_buffer();

            // One of the compositors (the oldest one) gets a new buffer...
            int oldest = i & 1;
            q.compositor_release(compositor[oldest]);

            compositor[oldest] = q.compositor_acquire(this);
        }

        q.compositor_release(compositor[0]);
        q.compositor_release(compositor[1]);
    }
}

TEST_F(BufferQueueTest, framedropping_clients_get_all_buffers)
{
    for (int nbuffers = 2; nbuffers <= max_nbuffers_to_test; ++nbuffers)
    {
        mc::BufferQueue q(nbuffers, allocator, basic_properties);
        q.allow_framedropping(true);

        int const nframes = 100;
        int max_ownable_buffers = nbuffers - 1;
        std::unordered_set<uint32_t> ids_acquired;
        for (int i = 0; i < nframes; ++i)
        {
            auto handle = client_acquire_async(q);
            ASSERT_THAT(handle->has_acquired_buffer(), Eq(true));
            ids_acquired.insert(handle->id().as_uint32_t());
            handle->release_buffer();
        }

        EXPECT_THAT(ids_acquired.size(), Eq(max_ownable_buffers));
    }
}

TEST_F(BufferQueueTest, waiting_clients_unblock_on_shutdown)
{
    for (int nbuffers = 2; nbuffers <= max_nbuffers_to_test; ++nbuffers)
    {
        mc::BufferQueue q(nbuffers, allocator, basic_properties);
        q.allow_framedropping(false);

        int const max_ownable_buffers = nbuffers - 1;

        for (int b = 0; b < max_ownable_buffers; b++)
        {
            auto handle = client_acquire_async(q);
            ASSERT_THAT(handle->has_acquired_buffer(), Eq(true));
            handle->release_buffer();
        }

        auto handle = client_acquire_async(q);
        ASSERT_THAT(handle->has_acquired_buffer(), Eq(false));

        q.force_requests_to_complete();

        EXPECT_THAT(handle->has_acquired_buffer(), Eq(true));
    }
}

TEST_F(BufferQueueTest, client_framerate_matches_compositor)
{
    for (int nbuffers = 2; nbuffers <= 3; nbuffers++)
    {
        mc::BufferQueue q(nbuffers, allocator, basic_properties);
        unsigned long client_frames = 0;
        const unsigned long compose_frames = 20;

        q.allow_framedropping(false);

        std::atomic<bool> done(false);

        mt::AutoJoinThread monitor1([&]
        {
            for (unsigned long frame = 0; frame != compose_frames+3; frame++)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));

                auto buf = q.compositor_acquire(this);
                q.compositor_release(buf);

                if (frame == compose_frames)
                {
                    // Tell the "client" to stop after compose_frames, but
                    // don't stop rendering immediately to avoid blocking
                    // if we rendered any twice
                    done.store(true);
                }
            }
        });

        auto handle = client_acquire_async(q);
        ASSERT_THAT(handle->has_acquired_buffer(), Eq(true));
        handle->release_buffer();

        while (!done.load())
        {
            auto handle = client_acquire_async(q);
            handle->wait_for(std::chrono::seconds(1));
            ASSERT_THAT(handle->has_acquired_buffer(), Eq(true));
            handle->release_buffer();
            client_frames++;
        }

        monitor1.stop();

        // Roughly compose_frames == client_frames within 50%
        ASSERT_THAT(client_frames, Gt(compose_frames / 2));
        ASSERT_THAT(client_frames, Lt(compose_frames * 3 / 2));
    }
}

/* Regression test LP: #1241369 / LP: #1241371 */
TEST_F(BufferQueueTest, slow_client_framerate_matches_compositor)
{
    /* BufferQueue can only satify this for nbuffers >= 3
     * since a client can only own nbuffers - 1 at any one time
     */
    for (int nbuffers = 3; nbuffers <= 3; nbuffers++)
    {
        mc::BufferQueue q(nbuffers, allocator, basic_properties);
        unsigned long client_frames = 0;
        unsigned long const compose_frames = 100;
        auto const frame_time = std::chrono::milliseconds(16);

        q.allow_framedropping(false);

        std::atomic<bool> done(false);
        std::mutex sync;

        mt::AutoJoinThread monitor1([&]
        {
            for (unsigned long frame = 0; frame != compose_frames+3; frame++)
            {
                std::this_thread::sleep_for(frame_time);
                sync.lock();
                auto buf = q.compositor_acquire(this);
                q.compositor_release(buf);
                sync.unlock();

                if (frame == compose_frames)
                {
                    // Tell the "client" to stop after compose_frames, but
                    // don't stop rendering immediately to avoid blocking
                    // if we rendered any twice
                    done.store(true);
                }
            }
        });

        auto handle = client_acquire_async(q);
        ASSERT_THAT(handle->has_acquired_buffer(), Eq(true));
        handle->release_buffer();

        while (!done.load())
        {
            sync.lock();
            sync.unlock();
            auto handle = client_acquire_async(q);
            handle->wait_for(std::chrono::seconds(1));
            ASSERT_THAT(handle->has_acquired_buffer(), Eq(true));
            std::this_thread::sleep_for(frame_time);
            handle->release_buffer();
            client_frames++;
        }

        monitor1.stop();

        // Roughly compose_frames == client_frames within 20%
        ASSERT_THAT(client_frames, Gt(compose_frames * 0.8f));
        ASSERT_THAT(client_frames, Lt(compose_frames * 1.2f));
    }
}

TEST_F(BufferQueueTest, resize_affects_client_acquires_immediately)
{
    for (int nbuffers = 1; nbuffers <= max_nbuffers_to_test; ++nbuffers)
    {
        mc::BufferQueue q(nbuffers, allocator, basic_properties);

        for (int width = 1; width < 100; ++width)
        {
            const geom::Size expect_size{width, width * 2};

            for (int subframe = 0; subframe < 3; ++subframe)
            {
                q.resize(expect_size);
                auto handle = client_acquire_async(q);
                ASSERT_THAT(handle->has_acquired_buffer(), Eq(true));
                auto buffer = handle->buffer();
                ASSERT_THAT(expect_size, Eq(buffer->size()));
                handle->release_buffer();

                auto comp_buffer = q.compositor_acquire(this);
                ASSERT_THAT(expect_size, Eq(comp_buffer->size()));
                q.compositor_release(comp_buffer);
            }
        }
    }
}

TEST_F(BufferQueueTest, compositor_acquires_resized_frames)
{
    for (int nbuffers = 2; nbuffers <= max_nbuffers_to_test; ++nbuffers)
    {
        mc::BufferQueue q(nbuffers, allocator, basic_properties);
        mg::BufferID history[5];

        const int width0 = 123;
        const int height0 = 456;
        const int dx = 2;
        const int dy = -3;
        int width = width0;
        int height = height0;

        for (int produce = 0; produce < nbuffers - 1; ++produce)
        {
            geom::Size new_size{width, height};
            width += dx;
            height += dy;

            q.resize(new_size);
            auto handle = client_acquire_async(q);
            ASSERT_THAT(handle->has_acquired_buffer(), Eq(true));
            history[produce] = handle->id();
            auto buffer = handle->buffer();
            ASSERT_THAT(new_size, Eq(buffer->size()));
            handle->release_buffer();
        }

        width = width0;
        height = height0;

        for (int consume = 0; consume < nbuffers - 1; ++consume)
        {
            geom::Size expect_size{width, height};
            width += dx;
            height += dy;

            auto buffer = q.compositor_acquire(this);

            // Verify the compositor gets resized buffers, eventually
            ASSERT_THAT(expect_size, Eq(buffer->size()));

            // Verify the compositor gets buffers with *contents*, ie. that
            // they have not been resized prematurely and are empty.
            ASSERT_THAT(history[consume], Eq(buffer->id()));

            q.compositor_release(buffer);
        }

        // Verify the final buffer size sticks
        const geom::Size final_size{width - dx, height - dy};
        for (int unchanging = 0; unchanging < 100; ++unchanging)
        {
            auto buffer = q.compositor_acquire(this);
            ASSERT_THAT(final_size, Eq(buffer->size()));
            q.compositor_release(buffer);
        }
    }
}

/* Regression test for LP: #1306464 */
TEST_F(BufferQueueTest, framedropping_client_acquire_does_not_block_when_no_available_buffers)
{
    using namespace testing;

    int const nbuffers{3};

    mc::BufferQueue q{nbuffers, allocator, basic_properties};
    q.allow_framedropping(true);

    std::vector<std::shared_ptr<mg::Buffer>> buffers;

    /* The client can never own this acquired buffer */
    auto comp_buffer = q.compositor_acquire(this);
    buffers.push_back(comp_buffer);

    /* Let client release all possible buffers so they go into
     * the ready queue
     */
    for (int i = 0; i < nbuffers; ++i)
    {
        auto handle = client_acquire_async(q);
        ASSERT_THAT(handle->has_acquired_buffer(), Eq(true));
        /* Check the client never got the compositor buffer acquired above */
        ASSERT_THAT(handle->id(), Ne(comp_buffer->id()));
        handle->release_buffer();
    }

    /* Let the compositor acquire all ready buffers */
    for (int i = 0; i < nbuffers; ++i)
    {
        buffers.push_back(q.compositor_acquire(this));
    }

    /* At this point the queue has 0 free buffers and 0 ready buffers
     * so the next client request should not be satisfied until
     * a compositor releases its buffers */
    auto handle = client_acquire_async(q);
    EXPECT_THAT(handle->has_acquired_buffer(), Eq(false));

    /* Release compositor buffers so that the client can get one */
    for (auto const& buffer : buffers)
    {
        q.compositor_release(buffer);
    }

    EXPECT_THAT(handle->has_acquired_buffer(), Eq(true));
}

TEST_F(BufferQueueTest, compositor_never_owns_client_buffers)
{
    for (int nbuffers = 2; nbuffers <= max_nbuffers_to_test; ++nbuffers)
    {
        mc::BufferQueue q(nbuffers, allocator, basic_properties);

        std::mutex client_buffer_guard;
        mg::Buffer* client_buffer = nullptr;
        std::atomic<bool> done(false);

        auto unblock = [&done]{ done = true; };
        mt::AutoUnblockThread compositor_thread(unblock, [&]
        {
            while (!done)
            {
                auto buffer = q.compositor_acquire(this);

                {
                    std::lock_guard<std::mutex> lock(client_buffer_guard);
                    if (client_buffer)
                        ASSERT_THAT(buffer->id(), Ne(client_buffer->id()));
                }

                std::this_thread::yield();
                q.compositor_release(buffer);
            }
        });

        for (int i = 0; i < 1000; ++i)
        {
            auto handle = client_acquire_async(q);
            handle->wait_for(std::chrono::seconds(1));
            ASSERT_THAT(handle->has_acquired_buffer(), Eq(true));

            {
                std::lock_guard<std::mutex> lock(client_buffer_guard);
                client_buffer = handle->buffer();
            }

            std::this_thread::yield();

            std::lock_guard<std::mutex> lock(client_buffer_guard);
            handle->release_buffer();
            client_buffer = nullptr;
        }
    }
}

TEST_F(BufferQueueTest, client_never_owns_compositor_buffers)
{
    for (int nbuffers = 2; nbuffers <= max_nbuffers_to_test; ++nbuffers)
    {
        mc::BufferQueue q(nbuffers, allocator, basic_properties);
        for (int i = 0; i < 100; ++i)
        {
            auto handle = client_acquire_async(q);
            ASSERT_THAT(handle->has_acquired_buffer(), Eq(true));

            auto client_id = handle->id();
            std::vector<std::shared_ptr<mg::Buffer>> buffers;
            for (int j = 0; j < nbuffers; j++)
            {
                auto buffer = q.compositor_acquire(this);
                ASSERT_THAT(client_id, Ne(buffer->id()));
                buffers.push_back(buffer);
            }

            for (auto const& buffer: buffers)
                q.compositor_release(buffer);

            handle->release_buffer();

            /* Flush out one ready buffer */
            auto buffer = q.compositor_acquire(this);
            ASSERT_THAT(client_id, Eq(buffer->id()));
            q.compositor_release(buffer);
        }
    }
}

/* Regression test for an issue brought up at:
 * http://code.launchpad.net/~albaguirre/mir/
 * alternative-switching-bundle-implementation/+merge/216606/comments/517048
 */
TEST_F(BufferQueueTest, buffers_are_not_lost)
{
    for (int nbuffers = 3; nbuffers <= max_nbuffers_to_test; ++nbuffers)
    {
        mc::BufferQueue q(nbuffers, allocator, basic_properties);

        void const* main_compositor = reinterpret_cast<void const*>(0);
        void const* second_compositor = reinterpret_cast<void const*>(1);

        /* Hold a reference to current compositor buffer*/
        auto comp_buffer1 = q.compositor_acquire(main_compositor);

        /* Make nbuffers -1 ready to composite */
        int const max_ownable_buffers = nbuffers - 1;
        for (int acquires = 0; acquires < max_ownable_buffers; ++acquires)
        {
            auto handle = client_acquire_async(q);
            ASSERT_THAT(handle->has_acquired_buffer(), Eq(true));
            handle->release_buffer();
        }

        /* Have a second compositor advance the current compositor buffer at least twice */
        for (int acquires = 0; acquires < nbuffers; ++acquires)
        {
            auto comp_buffer = q.compositor_acquire(second_compositor);
            q.compositor_release(comp_buffer);
        }
        q.compositor_release(comp_buffer1);

        /* An async client should still be able to cycle through all the available buffers */
        std::atomic<bool> done(false);
        auto unblock = [&done] { done = true; };
        mt::AutoUnblockThread compositor(unblock,
           compositor_thread, std::ref(q), std::ref(done));

        std::unordered_set<mg::Buffer *> unique_buffers_acquired;
        for (int frame = 0; frame < max_ownable_buffers*2; frame++)
        {
            std::vector<mg::Buffer *> client_buffers;
            for (int acquires = 0; acquires < max_ownable_buffers; ++acquires)
            {
                auto handle = client_acquire_async(q);
                handle->wait_for(std::chrono::seconds(1));
                ASSERT_THAT(handle->has_acquired_buffer(), Eq(true));
                unique_buffers_acquired.insert(handle->buffer());
                client_buffers.push_back(handle->buffer());
            }

            for (auto const& buffer : client_buffers)
            {
                q.client_release(buffer);
            }
        }

        EXPECT_THAT(unique_buffers_acquired.size(), Eq(nbuffers));

    }
}

/* FIXME (enabling this optimization breaks timing tests) */
TEST_F(BufferQueueTest, DISABLED_synchronous_clients_only_get_two_real_buffers)
{
    for (int nbuffers = 2; nbuffers <= max_nbuffers_to_test; ++nbuffers)
    {
        mc::BufferQueue q(nbuffers, allocator, basic_properties);
        q.allow_framedropping(false);

        std::atomic<bool> done(false);
        auto unblock = [&done] { done = true; };
        mt::AutoUnblockThread compositor(unblock,
           compositor_thread, std::ref(q), std::ref(done));

        std::unordered_set<mg::Buffer *> buffers_acquired;

        for (int frame = 0; frame < 100; frame++)
        {
            auto handle = client_acquire_async(q);
            handle->wait_for(std::chrono::seconds(1));
            ASSERT_THAT(handle->has_acquired_buffer(), Eq(true));

            buffers_acquired.insert(handle->buffer());
            handle->release_buffer();
        }

        EXPECT_THAT(buffers_acquired.size(), Eq(2));
    }
}
